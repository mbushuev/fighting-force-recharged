using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Web.Script.Serialization;
using FightingForceSetup;
static class InstallerTests {
    static int failures;
    static void Check(bool value,string name) {if(!value) {Console.WriteLine("FAIL: "+name);++failures;}}
    static void Reject(Action action,string name) {try {action();Check(false,name);}catch(InvalidDataException) {}}
    static Dictionary<string,string> Inventory(string root) {
        return Directory.GetFiles(root,"*",SearchOption.AllDirectories).ToDictionary(
            f=>f.Substring(root.Length),f=>InstallerCore.HashFile(f)+":"+File.GetLastWriteTimeUtc(f).Ticks,StringComparer.OrdinalIgnoreCase);
    }
    static void Same(Dictionary<string,string> before,string root,string name) {
        var after=Inventory(root);Check(before.Count==after.Count && before.All(f=>after.ContainsKey(f.Key) && after[f.Key]==f.Value),name);
        Check(Directory.GetDirectories(root,".ff-fix-*").Length==0,name+" private staging cleanup");
    }
    static void FolderFix(string source,string parent,InstallerCore core) {
        string target=Path.Combine(parent,"existing-game");Directory.CreateDirectory(target);
        foreach(var file in core.FolderManifest.files) {
            string destination=InstallerCore.SafePath(target,file.path);Directory.CreateDirectory(Path.GetDirectoryName(destination));
            File.Copy(InstallerCore.SafePath(source,file.path),destination);
        }
        File.WriteAllText(Path.Combine(target,"fightingforce-compat.ini"),"[Camera]\nDistance=901\n");
        File.WriteAllText(Path.Combine(target,"dgVoodoo.conf"),"; existing configuration fixture\n");
        File.SetAttributes(Path.Combine(target,"dgVoodoo.conf"),FileAttributes.ReadOnly);
        File.WriteAllText(Path.Combine(target,"save-fixture.sav"),"existing save fixture");
        var pristine=Inventory(target);
        // An unmanaged DLL must fail before any other wrapper file is installed.
        string input=Path.Combine(target,"DINPUT.dll");File.WriteAllText(input,"foreign wrapper fixture");
        var foreign=Inventory(target);bool refused=false;
        try {core.Apply(target,false);}catch(IOException) {refused=true;}
        Check(refused,"foreign wrapper refusal");Same(foreign,target,"foreign wrapper unchanged");File.Delete(input);
        Directory.CreateDirectory(input);refused=false;
        try {core.Apply(target,false);}catch(IOException) {refused=true;}
        Check(refused && Directory.Exists(input),"folder at DLL path refusal");Same(pristine,target,"DLL path conflict preserves files");Directory.Delete(input,false);
        using(var stop=new CancellationTokenSource()) {
            bool cancelled=false;
            var installer=new InstallerCore((value,text)=>{if(text.StartsWith("Installed compatibility file:"))stop.Cancel();},stop.Token);
            try {installer.Apply(target,false);}catch(OperationCanceledException) {cancelled=true;}
            Check(cancelled,"cancel after first wrapper write");Same(pristine,target,"cancel preserves existing folder");
        }
        core.Apply(target,false);
        Check(File.ReadAllText(Path.Combine(target,"fightingforce-compat.ini")).Contains("Distance=901"),"preserve wrapper settings");
        Check(File.ReadAllText(Path.Combine(target,"dgVoodoo.conf")).Contains("existing configuration fixture"),"preserve runtime settings");
        // Simulate a previously recorded wrapper revision, never a game file.
        string receiptPath=Path.Combine(target,"Compatibility","wrapper-files.json");
        var serializer=new JavaScriptSerializer();var receipt=serializer.Deserialize<WrapperReceipt>(File.ReadAllText(receiptPath));
        File.WriteAllText(input,"previous project wrapper fixture");receipt.files["DINPUT.dll"]=InstallerCore.HashFile(input);
        File.WriteAllText(receiptPath,serializer.Serialize(receipt));var previous=Inventory(target);
        bool failed=false;
        var fault=new InstallerCore((value,text)=>{if(text.StartsWith("Installed compatibility file:"))throw new IOException("Injected installation failure.");},CancellationToken.None);
        try {fault.Apply(target,false);}catch(IOException) {failed=true;}
        Check(failed,"failure after replacing owned wrapper");Same(previous,target,"failure restores previous wrapper and receipt");
        using(var locked=new FileStream(input,FileMode.Open,FileAccess.Read,FileShare.Read)) {
            bool blocked=false;try {core.Apply(target,false);}catch(IOException) {blocked=true;}
            Check(blocked,"locked wrapper refusal");Same(previous,target,"locked wrapper leaves files unchanged");
        }
        core.Apply(target,false);Check(InstallerCore.HashFile(input)!=receipt.files["DINPUT.dll"],"update recorded wrapper");
        var installed=Inventory(target);core.Apply(target,false);Same(installed,target,"repeat apply preserves installed files");
        foreach(var file in core.FolderManifest.files)Check(InstallerCore.HashFile(InstallerCore.SafePath(target,file.path))==file.sha256,"original file preserved");
    }
    static int Main(string[] args) {
        string root=Path.Combine(Path.GetTempPath(),"ff-safe-path-tests");
        string volume=Path.GetPathRoot(root);Check(InstallerCore.FullDirectory(volume)==volume,"retain absolute volume root");
        Check(InstallerCore.FullDirectory(root+Path.DirectorySeparatorChar)==root,"normalize folder suffix");
        foreach(string bad in new[]{"../outside","/outside","a/../../outside","a//b","a/./b","file:stream","a./b","a /b"})
            Reject(()=>InstallerCore.SafePath(root,bad),bad);
        Check(InstallerCore.SafePath(root,"Wads/ENGLISH.WAD")==Path.Combine(root,"Wads","ENGLISH.WAD"),"normal relative path");
        var core=new InstallerCore(null,CancellationToken.None);Check(core.Manifest.files.Length==235,"retail file count");
        Check(core.FolderManifest.files.Length==235,"reviewed RIP file count");
        var manifest=new RetailManifest {schema=1,files=new[]{
            new RetailFile {path="file",source="file",origin="media",size=1,sha256=new string('0',64)},
            new RetailFile {path="FILE",source="file",origin="media",size=1,sha256=new string('0',64)}}};
        Reject(()=>InstallerCore.ValidateManifest(manifest),"case-insensitive destination collision");
        manifest.files=new[]{new RetailFile {path="DINPUT.dll",source="file",origin="media",size=1,sha256=new string('0',64)}};
        Reject(()=>InstallerCore.ValidateManifest(manifest),"wrapper and retail name collision");
        if(args.Length==2) {
            if(!core.IsMedia(args[0])) {FolderFix(args[0],args[1],core);Console.WriteLine(failures==0?"Folder fix: PASS; settings, saves, conflicts, update, locks and rollback.":"Folder fix: FAIL");return failures==0?0:1;}
            string target=Path.Combine(args[1],"cancelled-copy");
            using(var stop=new CancellationTokenSource()) {
                bool cancelled=false;
                var installer=new InstallerCore((value,text)=>{if(value>=10)stop.Cancel();},stop.Token);
                try {installer.Install(args[0],target,false);}
                catch(OperationCanceledException) {cancelled=true;}
                Check(cancelled && !Directory.Exists(target),"cancel during copying leaves no destination");
                Check(Directory.GetDirectories(args[1],".ff-install-*").Length==0,"cancel removes only private staging");
            }
        }
        Console.WriteLine(failures==0?"Installer paths and retail manifest: PASS":"Installer core: FAIL");return failures==0?0:1;
    }
}
