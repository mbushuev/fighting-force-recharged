using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Threading;
using System.Web.Script.Serialization;

namespace FightingForceSetup {
    public sealed class RetailFile {
        public string path, origin, source, sha256;
        public long size;
    }
    public sealed class RetailManifest {
        public int schema;
        public string edition, cabinet_sha256;
        public long cabinet_size;
        public RetailFile[] files;
    }
    public sealed class InstallResult {
        public string Target, ShortcutWarning, Notice;
        public int OriginalFiles;
    }
    public sealed class InstallSource {
        public RetailManifest Manifest;
        public bool Folder;
        public string Notice;
    }
    public sealed class InstallerCore {
        public readonly RetailManifest Manifest;
        public readonly RetailManifest FolderManifest;
        readonly Action<int,string> progress;
        readonly CancellationToken cancel;
        static readonly string[] RuntimeFiles={"DINPUT.dll","DDraw.dll","D3DImm.dll","D3D9.dll","fightingforce-compat.ini","dgVoodoo.conf"};
        public InstallerCore(Action<int,string> report,CancellationToken token) {
            progress=report ?? delegate {};cancel=token;
            Manifest=ReadManifest("FightingForce.Retail.json");FolderManifest=ReadManifest("FightingForce.Rip.json");
        }
        static RetailManifest ReadManifest(string name) {
            RetailManifest manifest;
            using(var stream=Assembly.GetExecutingAssembly().GetManifestResourceStream(name))
            using(var reader=new StreamReader(stream))manifest=new JavaScriptSerializer().Deserialize<RetailManifest>(reader.ReadToEnd());
            ValidateManifest(manifest);return manifest;
        }
        public static string SafePath(string root,string relative) {
            if(String.IsNullOrWhiteSpace(relative) || Path.IsPathRooted(relative) || relative.IndexOf(':')>=0)
                throw new InvalidDataException("Invalid relative file path.");
            var parts=relative.Replace('\\','/').Split('/');
            if(parts.Any(p=>p.Length==0 || p=="." || p==".." || p.TrimEnd(' ','.')!=p))
                throw new InvalidDataException("Unsafe file path.");
            string prefix=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar;
            string result=Path.GetFullPath(Path.Combine(prefix,relative.Replace('/',Path.DirectorySeparatorChar)));
            if(!result.StartsWith(prefix,StringComparison.OrdinalIgnoreCase))throw new InvalidDataException("File escaped its installation folder.");
            return result;
        }
        public static void ValidateManifest(RetailManifest manifest) {
            if(manifest==null || manifest.schema!=1 || manifest.files==null || manifest.files.Length==0)
                throw new InvalidDataException("Unsupported retail manifest.");
            var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach(var file in manifest.files) {
                SafePath(Path.GetTempPath(),file.path);SafePath(Path.GetTempPath(),file.source);
                if(!seen.Add(file.path.Replace('\\','/')) || file.size<0 || file.sha256==null || file.sha256.Length!=64 ||
                   (file.origin!="archive" && file.origin!="media" && file.origin!="folder"))throw new InvalidDataException("Invalid retail manifest entry.");
            }
            if(RuntimeFiles.Any(f=>seen.Contains(f)) || seen.Contains("Compatibility"))
                throw new InvalidDataException("Compatibility files overlap original game files.");
        }
        public static string HashFile(string path) {
            using(var hash=SHA256.Create())using(var stream=File.OpenRead(path))
                return BitConverter.ToString(hash.ComputeHash(stream)).Replace("-","").ToLowerInvariant();
        }
        static void CheckFile(string path,long size,string hash) {
            if(!File.Exists(path) || new FileInfo(path).Length!=size || HashFile(path)!=hash)
                throw new InvalidDataException("Original file verification failed: "+Path.GetFileName(path));
        }
        public bool IsMedia(string root) {
            try {
                var cabinet=SafePath(root,"disk1/DATA1.CAB");
                return File.Exists(cabinet) && new FileInfo(cabinet).Length==Manifest.cabinet_size &&
                    File.Exists(SafePath(root,"Wads/ENGLISH.WAD")) && File.Exists(SafePath(root,"Intro/logo.rpl"));
            } catch(IOException) {return false;} catch(UnauthorizedAccessException) {return false;}
        }
        RetailManifest FolderProfile(string root) {
            string executable=SafePath(root,"fforce.exe");
            if(!File.Exists(executable))throw new InvalidDataException("Select the original English disc root or a supported game folder containing fforce.exe and Wads.");
            string hash=HashFile(executable);
            foreach(var profile in new[]{Manifest,FolderManifest}) {
                var expected=profile.files.First(f=>String.Equals(f.path,"fforce.exe",StringComparison.OrdinalIgnoreCase));
                if(new FileInfo(executable).Length==expected.size && hash==expected.sha256)return profile;
            }
            throw new InvalidDataException("This game folder contains a modified or unsupported fforce.exe. Only the reviewed English retail and RIP builds are supported. Setup does not replace or repair game files.");
        }
        public InstallSource InspectSource(string root) {
            if(IsMedia(root)) {
                CheckFile(SafePath(root,"disk1/DATA1.CAB"),Manifest.cabinet_size,Manifest.cabinet_sha256);
                return new InstallSource {Manifest=Manifest,Folder=false,Notice="Keep the original disc mounted for play and CD music."};
            }
            var profile=FolderProfile(root);
            foreach(var file in profile.files) {cancel.ThrowIfCancellationRequested();CheckFile(SafePath(root,file.path),file.size,file.sha256);}
            return new InstallSource {Manifest=profile,Folder=true,Notice=profile==FolderManifest?
                "RIP source preserved. Intro and CD music are not included; an original CD can supply music.":
                "Keep the original disc mounted for play and CD music."};
        }
        public string DetectMedia() {
            foreach(var drive in DriveInfo.GetDrives()) {
                try {if(drive.DriveType==DriveType.CDRom && drive.IsReady && IsMedia(drive.RootDirectory.FullName))return drive.RootDirectory.FullName;}
                catch(IOException) {} catch(UnauthorizedAccessException) {}
            }
            return "";
        }
        void Report(int value,string message) {cancel.ThrowIfCancellationRequested();progress(value,message);}
        static string Quote(string value) {
            // Windows argv quoting, including trailing backslashes before a quote.
            var result=new System.Text.StringBuilder("\"");int slashes=0;
            foreach(char c in value) {
                if(c=='\\') {++slashes;continue;}
                result.Append('\\',c=='"'?slashes*2+1:slashes);result.Append(c);slashes=0;
            }
            return result.Append('\\',slashes*2).Append('"').ToString();
        }
        static void DeletePrivateDirectory(string path,string parent,string prefix) {
            string absolute=Path.GetFullPath(path),expected=Path.GetFullPath(parent).TrimEnd('\\')+"\\";
            if(!absolute.StartsWith(expected,StringComparison.OrdinalIgnoreCase) ||
               !Path.GetFileName(absolute).StartsWith(prefix,StringComparison.Ordinal))throw new IOException("Cleanup path was not private staging.");
            if(Directory.Exists(absolute))Directory.Delete(absolute,true);
        }
        void ExtractPayload(string temporary) {
            using(var resource=Assembly.GetExecutingAssembly().GetManifestResourceStream("FightingForce.Payload.zip"))
            using(var zip=new ZipArchive(resource,ZipArchiveMode.Read)) {
                foreach(var entry in zip.Entries) {
                    cancel.ThrowIfCancellationRequested();
                    string destination=SafePath(temporary,entry.FullName);
                    if(entry.FullName.EndsWith("/")) {Directory.CreateDirectory(destination);continue;}
                    Directory.CreateDirectory(Path.GetDirectoryName(destination));
                    using(var source=entry.Open())using(var target=new FileStream(destination,FileMode.CreateNew))source.CopyTo(target);
                }
            }
        }
        void ExtractCabinet(string media,string temporary) {
            var arguments="-O -d "+Quote(Path.Combine(temporary,"retail"))+" x "+Quote(SafePath(media,"disk1/DATA1.CAB"));
            var info=new ProcessStartInfo(Path.Combine(temporary,"tools","unshield.exe"),arguments) {
                UseShellExecute=false,CreateNoWindow=true,WorkingDirectory=Path.Combine(temporary,"tools"),
                RedirectStandardOutput=true,RedirectStandardError=true
            };
            using(var process=Process.Start(info)) {
                var stdout=process.StandardOutput.ReadToEndAsync();var stderr=process.StandardError.ReadToEndAsync();
                while(!process.WaitForExit(100))if(cancel.IsCancellationRequested) {process.Kill();process.WaitForExit();cancel.ThrowIfCancellationRequested();}
                System.Threading.Tasks.Task.WaitAll(stdout,stderr);
                if(process.ExitCode!=0)throw new InvalidDataException("The original installer archive could not be extracted.");
            }
        }
        void CopyVerified(string source,string target,RetailFile file) {
            CheckFile(source,file.size,file.sha256);Directory.CreateDirectory(Path.GetDirectoryName(target));
            using(var input=File.OpenRead(source))using(var output=new FileStream(target,FileMode.CreateNew,FileAccess.Write)) {
                byte[] buffer=new byte[1024*1024];int count;
                while((count=input.Read(buffer,0,buffer.Length))>0) {cancel.ThrowIfCancellationRequested();output.Write(buffer,0,count);}
            }
            CheckFile(target,file.size,file.sha256);
        }
        public int Verify(string target) {return VerifyAt(target,FolderProfile(target),0,100);}
        int VerifyAt(string target,RetailManifest profile,int start,int span) {
            int count=0;
            foreach(var file in profile.files) {Report(start+count*span/profile.files.Length,"Verifying source files...");CheckFile(SafePath(target,file.path),file.size,file.sha256);++count;}
            Report(start+span,"All supplied game files are unchanged.");return count;
        }
        public InstallResult Install(string media,string target,bool shortcut) {
            if(!Environment.Is64BitOperatingSystem)throw new NotSupportedException("This installer requires 64-bit Windows 10 or later.");
            media=Path.GetFullPath(media);target=Path.GetFullPath(target).TrimEnd('\\');
            Report(0,"Checking the selected source...");var selected=InspectSource(media);var profile=selected.Manifest;
            if(Directory.Exists(target) || File.Exists(target))throw new IOException("Choose a new destination folder. Existing installations are never overwritten.");
            if(target.StartsWith(media.TrimEnd('\\')+"\\",StringComparison.OrdinalIgnoreCase))throw new IOException("The destination cannot be inside the source folder.");
            string parent=Path.GetDirectoryName(target);
            if(String.IsNullOrEmpty(parent) || !Directory.Exists(parent))throw new IOException("Select an existing parent folder on a writable drive.");
            var drive=new DriveInfo(Path.GetPathRoot(target));
            long needed=profile.files.Sum(f=>f.size)+64L*1024*1024;
            if(drive.IsReady && drive.AvailableFreeSpace<needed)throw new IOException("Not enough free space for the game and compatibility files.");
            string temporary=Path.Combine(Path.GetTempPath(),"FFSetup-"+Guid.NewGuid().ToString("N"));
            string staging=Path.Combine(parent,".ff-install-"+Guid.NewGuid().ToString("N"));
            bool committed=false;
            try {
                Directory.CreateDirectory(temporary);ExtractPayload(temporary);
                if(!selected.Folder) {Report(3,"Extracting the original English executables and libraries...");ExtractCabinet(media,temporary);}
                Directory.CreateDirectory(staging);int done=0;
                foreach(var file in profile.files) {
                    Report(5+done*85/profile.files.Length,"Copying and verifying "+file.path);
                    string source=selected.Folder?SafePath(media,file.path):
                        SafePath(file.origin=="archive"?Path.Combine(temporary,"retail"):media,file.source);
                    CopyVerified(source,SafePath(staging,file.path),file);++done;
                }
                Report(92,"Adding the compatibility wrapper...");
                foreach(string name in RuntimeFiles)File.Copy(SafePath(Path.Combine(temporary,"game"),name),SafePath(staging,name),false);
                foreach(string source in Directory.GetFiles(Path.Combine(temporary,"support"),"*",SearchOption.AllDirectories)) {
                    string relative=source.Substring(Path.Combine(temporary,"support").Length+1);
                    string destination=SafePath(staging,"Compatibility/"+relative.Replace('\\','/'));
                    Directory.CreateDirectory(Path.GetDirectoryName(destination));File.Copy(source,destination,false);
                }
                VerifyAt(staging,profile,94,5);Report(99,"Completing the installation...");
                Directory.Move(staging,target);committed=true;
                var result=new InstallResult {Target=target,OriginalFiles=profile.files.Length,Notice=selected.Notice};
                if(shortcut)try {CreateShortcut(target);}catch(Exception e) {result.ShortcutWarning="The game was installed, but a shortcut could not be created: "+e.Message;}
                progress(100,"Installed. "+selected.Notice);return result;
            } finally {
                try {if(!committed)DeletePrivateDirectory(staging,parent,".ff-install-");}catch(IOException) {} catch(UnauthorizedAccessException) {}
                try {DeletePrivateDirectory(temporary,Path.GetTempPath(),"FFSetup-");}catch(IOException) {} catch(UnauthorizedAccessException) {}
            }
        }
        static void CreateShortcut(string target) {
            Type shell=Type.GetTypeFromProgID("WScript.Shell");object app=Activator.CreateInstance(shell),link=null;
            try {
                string desktop=Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory);
                string path=Path.Combine(desktop,"Fighting Force (Compatibility).lnk");int suffix=2;
                while(File.Exists(path))path=Path.Combine(desktop,"Fighting Force (Compatibility) "+(suffix++)+".lnk");
                link=shell.InvokeMember("CreateShortcut",BindingFlags.InvokeMethod,null,app,new object[]{path});
                foreach(var pair in new[]{new[]{"TargetPath",Path.Combine(target,"fforce.exe")},new[]{"WorkingDirectory",target},new[]{"IconLocation",Path.Combine(target,"FFORCE.ICO")}})
                    link.GetType().InvokeMember(pair[0],BindingFlags.SetProperty,null,link,new object[]{pair[1]});
                link.GetType().InvokeMember("Save",BindingFlags.InvokeMethod,null,link,null);
            } finally {if(link!=null)System.Runtime.InteropServices.Marshal.FinalReleaseComObject(link);System.Runtime.InteropServices.Marshal.FinalReleaseComObject(app);}
        }
    }
}
