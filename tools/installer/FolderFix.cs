using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading;
using System.Web.Script.Serialization;

namespace FightingForceSetup {
    public sealed class WrapperReceipt {
        public int schema;
        public string product;
        public Dictionary<string,string> files;
    }
    public sealed partial class InstallerCore {
        const string ReceiptPath="Compatibility/wrapper-files.json";
        sealed class WrapperChange {
            public string Relative, Source, Destination, Backup;
            public bool Replacing, BackedUp, Installed;
        }
        static bool KeepSettings(string path) {
            return String.Equals(path,"fightingforce-compat.ini",StringComparison.OrdinalIgnoreCase) ||
                String.Equals(path,"dgVoodoo.conf",StringComparison.OrdinalIgnoreCase);
        }
        static void RecordNewWrapper(string target) {
            var files=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
            foreach(string name in RuntimeFiles)files.Add(name,HashFile(SafePath(target,name)));
            string support=Path.Combine(target,"Compatibility");
            foreach(string path in Directory.GetFiles(support,"*",SearchOption.AllDirectories)) {
                string relative="Compatibility/"+path.Substring(support.Length+1).Replace('\\','/');
                if(!String.Equals(relative,ReceiptPath,StringComparison.OrdinalIgnoreCase))files.Add(relative,HashFile(path));
            }
            File.WriteAllText(SafePath(target,ReceiptPath),new JavaScriptSerializer().Serialize(
                new WrapperReceipt {schema=1,product="FightingForceCompatibility",files=files}));
        }
        static void CheckWrapperPath(string root,string relative) {
            string destination=SafePath(root,relative),current=root;
            var parts=relative.Replace('\\','/').Split('/');
            for(int i=0;i<parts.Length-1;++i) {
                current=Path.Combine(current,parts[i]);
                if(File.Exists(current))throw new IOException("A compatibility folder name is occupied by a file.");
                if(Directory.Exists(current) && (File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)
                    throw new IOException("Compatibility folders cannot be symbolic links or junctions.");
            }
            if(Directory.Exists(destination) || (File.Exists(destination) &&
                (File.GetAttributes(destination)&FileAttributes.ReparsePoint)!=0))
                throw new IOException("A compatibility file name is occupied by a folder or link.");
        }
        static Dictionary<string,string> ReadWrapperReceipt(string target) {
            CheckWrapperPath(target,ReceiptPath);string path=SafePath(target,ReceiptPath);
            var result=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
            if(!File.Exists(path))return result;
            WrapperReceipt receipt;
            try {receipt=new JavaScriptSerializer().Deserialize<WrapperReceipt>(File.ReadAllText(path));}
            catch(ArgumentException) {throw new InvalidDataException("The compatibility installation record is invalid.");}
            if(receipt==null || receipt.schema!=1 || receipt.product!="FightingForceCompatibility" || receipt.files==null)
                throw new InvalidDataException("The compatibility installation record is unsupported.");
            foreach(var file in receipt.files) {
                SafePath(target,file.Key);
                if(file.Value==null || file.Value.Length!=64 || file.Value.Any(c=>!Uri.IsHexDigit(c)) || result.ContainsKey(file.Key))
                    throw new InvalidDataException("The compatibility installation record is invalid.");
                result.Add(file.Key,file.Value);
            }
            return result;
        }
        static void EnsureWrapperDirectory(string path,List<string> created) {
            if(Directory.Exists(path))return;
            EnsureWrapperDirectory(Path.GetDirectoryName(path),created);
            Directory.CreateDirectory(path);created.Add(path);
        }
        public InstallResult Apply(string target,bool shortcut) {
            if(!Environment.Is64BitOperatingSystem)throw new NotSupportedException("This installer requires 64-bit Windows 10 or later.");
            target=FullDirectory(target);
            Report(0,"Verifying the existing game folder...");
            // This is intentionally a folder-only boundary. Never copy, repair or
            // stage a supplied game file while adding the separate fix.
            var profile=FolderProfile(target);VerifyAt(target,profile,0,25);
            var previous=ReadWrapperReceipt(target);
            string temporary=Path.Combine(Path.GetTempPath(),"FFSetup-"+Guid.NewGuid().ToString("N"));
            string staging=Path.Combine(target,".ff-fix-"+Guid.NewGuid().ToString("N"));
            var changes=new List<WrapperChange>();var created=new List<string>();
            bool committed=false,rollbackFailed=false;
            try {
                Directory.CreateDirectory(temporary);ExtractPayload(temporary);
                var payload=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
                foreach(string name in RuntimeFiles)payload.Add(name,SafePath(Path.Combine(temporary,"game"),name));
                string support=Path.Combine(temporary,"support");
                foreach(string file in Directory.GetFiles(support,"*",SearchOption.AllDirectories))
                    payload.Add("Compatibility/"+file.Substring(support.Length+1).Replace('\\','/'),file);
                var protectedFiles=new HashSet<string>(profile.files.Select(f=>f.path.Replace('\\','/')),StringComparer.OrdinalIgnoreCase);
                var receipt=new WrapperReceipt {schema=1,product="FightingForceCompatibility",files=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase)};
                foreach(var file in payload) {
                    if(protectedFiles.Contains(file.Key))throw new InvalidDataException("Compatibility files overlap game files.");
                    CheckWrapperPath(target,file.Key);string destination=SafePath(target,file.Key),hash=HashFile(file.Value);
                    bool exists=File.Exists(destination);
                    if(exists) {
                        string actual=HashFile(destination),owned;
                        if(KeepSettings(file.Key)) {receipt.files.Add(file.Key,actual);continue;}
                        if(actual==hash) {receipt.files.Add(file.Key,hash);continue;}
                        if(!previous.TryGetValue(file.Key,out owned) || actual!=owned)
                            throw new IOException("Existing compatibility file is unrecognized or locally changed: "+file.Key+". It was left unchanged.");
                    }
                    changes.Add(new WrapperChange {Relative=file.Key,Source=file.Value,Destination=destination,Replacing=exists});
                    receipt.files.Add(file.Key,hash);
                }
                Directory.CreateDirectory(staging);
                string record=Path.Combine(staging,"receipt.json");
                File.WriteAllText(record,new JavaScriptSerializer().Serialize(receipt));
                string receiptTarget=SafePath(target,ReceiptPath);bool receiptExists=File.Exists(receiptTarget);
                if(!receiptExists || HashFile(receiptTarget)!=HashFile(record))
                    changes.Add(new WrapperChange {Relative=ReceiptPath,Source=record,Destination=receiptTarget,Replacing=receiptExists});
                // Refuse locked/read-only replacement files before installing anything.
                foreach(var change in changes.Where(c=>c.Replacing))
                    using(var check=new FileStream(change.Destination,FileMode.Open,FileAccess.ReadWrite,FileShare.None)) {}
                int done=0;
                foreach(var change in changes) {
                    Report(30+done*35/changes.Count,"Preparing compatibility files...");
                    string incoming=SafePath(staging,"incoming/"+change.Relative);
                    Directory.CreateDirectory(Path.GetDirectoryName(incoming));File.Copy(change.Source,incoming,false);change.Source=incoming;
                    change.Backup=SafePath(staging,"previous/"+change.Relative);++done;
                }
                done=0;
                foreach(var change in changes) {
                    cancel.ThrowIfCancellationRequested();CheckWrapperPath(target,change.Relative);
                    EnsureWrapperDirectory(Path.GetDirectoryName(change.Destination),created);
                    if(change.Replacing) {
                        Directory.CreateDirectory(Path.GetDirectoryName(change.Backup));
                        File.Move(change.Destination,change.Backup);change.BackedUp=true;
                    }
                    File.Move(change.Source,change.Destination);change.Installed=true;++done;
                    Report(65+done*25/changes.Count,"Installed compatibility file: "+change.Relative);
                }
                VerifyAt(target,profile,90,9);cancel.ThrowIfCancellationRequested();committed=true;
                var result=new InstallResult {Target=target,OriginalFiles=profile.files.Length,
                    Notice="Fix installed in the existing folder. Game files and existing settings were preserved."};
                if(shortcut)try {CreateShortcut(target);}catch(Exception e) {result.ShortcutWarning="The fix was installed, but a shortcut could not be created: "+e.Message;}
                progress(100,result.Notice);return result;
            } finally {
                if(!committed) {
                    try {
                        foreach(var change in changes.AsEnumerable().Reverse()) {
                            if(change.Installed)File.Delete(change.Destination);
                            if(change.BackedUp)File.Move(change.Backup,change.Destination);
                        }
                        foreach(string directory in created.AsEnumerable().Reverse())Directory.Delete(directory,false);
                    } catch(IOException) {rollbackFailed=true;} catch(UnauthorizedAccessException) {rollbackFailed=true;}
                }
                if(!rollbackFailed)DeletePrivateDirectory(staging,target,".ff-fix-");
                try {DeletePrivateDirectory(temporary,Path.GetTempPath(),"FFSetup-");}catch(IOException) {} catch(UnauthorizedAccessException) {}
                if(rollbackFailed)throw new IOException("Compatibility rollback could not finish. Its private .ff-fix folder was retained for recovery; game files were not modified.");
            }
        }
    }
}
