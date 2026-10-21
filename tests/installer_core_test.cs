using System;
using System.IO;
using System.Threading;
using FightingForceSetup;
static class InstallerTests {
    static int failures;
    static void Check(bool value,string name) {if(!value) {Console.WriteLine("FAIL: "+name);++failures;}}
    static void Reject(Action action,string name) {try {action();Check(false,name);}catch(InvalidDataException) {}}
    static int Main(string[] args) {
        string root=Path.Combine(Path.GetTempPath(),"ff-safe-path-tests");
        foreach(string bad in new[]{"../outside","/outside","a/../../outside","a//b","a/./b","file:stream","a./b","a /b"})
            Reject(()=>InstallerCore.SafePath(root,bad),bad);
        Check(InstallerCore.SafePath(root,"Wads/ENGLISH.WAD")==Path.Combine(root,"Wads","ENGLISH.WAD"),"normal relative path");
        var core=new InstallerCore(null,CancellationToken.None);Check(core.Manifest.files.Length==235,"retail file count");
        var manifest=new RetailManifest {schema=1,files=new[]{
            new RetailFile {path="file",source="file",origin="media",size=1,sha256=new string('0',64)},
            new RetailFile {path="FILE",source="file",origin="media",size=1,sha256=new string('0',64)}}};
        Reject(()=>InstallerCore.ValidateManifest(manifest),"case-insensitive destination collision");
        manifest.files=new[]{new RetailFile {path="DINPUT.dll",source="file",origin="media",size=1,sha256=new string('0',64)}};
        Reject(()=>InstallerCore.ValidateManifest(manifest),"wrapper and retail name collision");
        if(args.Length==2) {
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
