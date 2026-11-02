using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Windows.Forms;
using FightingForceSetup;

static class InstallerLayoutTests {
    static int failures;
    static void Check(bool value,string name) {if(!value) {Console.WriteLine("FAIL: "+name);++failures;}}
    static Control Find(Control form,string name) {return form.Controls.Find(name,true)[0];}
    static IEnumerable<Control> Children(Control root) {
        foreach(Control child in root.Controls) {yield return child;foreach(var descendant in Children(child))yield return descendant;}
    }
    static void Layout(Control root) {
        for(int i=0;i<4;++i) {root.PerformLayout();foreach(var child in Children(root))child.PerformLayout();}
    }
    static void Verify(Form form,string scenario) {
        Layout(form);
        bool folder=((ComboBox)Find(form,"Mode")).SelectedIndex==0;
        var targetRow=Find(form,"Target").Parent;
        foreach(var control in Children(form)) {
            if(folder && (control.Name=="TargetLabel" || control==targetRow || control.Parent==targetRow))continue;
            if(control is Label) {
                var needed=TextRenderer.MeasureText(control.Text,control.Font,new Size(control.Width,Int32.MaxValue),TextFormatFlags.WordBreak|TextFormatFlags.NoPrefix|TextFormatFlags.TextBoxControl);
                Check(needed.Height<=control.Height,scenario+" wraps "+control.Name);
                Check(control.Left>=0 && control.Right<=control.Parent.ClientSize.Width,scenario+" contains "+control.Name);
            }
            if(control is Button) {
                var needed=TextRenderer.MeasureText(control.Text,control.Font);
                Check(control.Width>=needed.Width+control.Padding.Horizontal && control.Height>=needed.Height+control.Padding.Vertical,scenario+" fits button "+control.Name);
            }
        }
        var actions=Find(form,"Actions");
        Check(actions.Parent.RectangleToScreen(actions.Bounds).Bottom<=form.RectangleToScreen(form.ClientRectangle).Bottom,scenario+" exposes action row");
        Check(Find(form,"Progress").Height>0 && Find(form,"Details").Height>0,scenario+" retains progress and details");
        var body=(Panel)Find(form,"SetupBody");var content=Find(form,"SetupContent");
        Check(!body.HorizontalScroll.Visible,scenario+" has no horizontal overflow");
        Check(content.Width<=body.ClientSize.Width,scenario+" fits content width");
        if(content.Height>body.ClientSize.Height)Check(body.VerticalScroll.Visible,scenario+" scrolls tall content");
        foreach(var row in new[]{"Source","Target"}) {
            if(folder && row=="Target")continue;
            var path=Find(form,row);var browse=Find(form,row+"Browse");
            Check(path.Width>0 && path.Right<=browse.Left && browse.Right<=browse.Parent.ClientSize.Width,scenario+" fits path row "+row);
        }
    }
    [STAThread] static int Main(string[] args) {
        Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
        foreach(float scale in new[]{1f,1.25f,1.5f,1.75f,2f,2.5f,3f}) {
            using(var form=new SetupForm()) {
                var handle=form.Handle;Layout(form);
                float systemScale=form.CurrentAutoScaleDimensions.Width/96f;
                // Exercise proportional device geometry and point fonts without changing OS settings.
                form.AutoScaleMode=AutoScaleMode.None;form.Scale(new SizeF(scale/systemScale,scale/systemScale));
                form.Font=new Font("Segoe UI",10*scale/systemScale);Find(form,"Title").Font=new Font(form.Font,FontStyle.Bold);
                string scenario=(scale*100).ToString("0")+" percent";
                var mode=(ComboBox)Find(form,"Mode");mode.SelectedIndex=1;
                form.ClientSize=new Size((int)(760*scale),(int)(580*scale));Verify(form,scenario);
                Find(form,"Status").Text="Copying and verifying the original game files. The installation is still in progress; keep the source disc mounted until verification completes.";
                Find(form,"Source").Text=Path.Combine(Path.GetTempPath(),new string('s',120));
                Find(form,"Target").Text=Path.Combine(Path.GetTempPath(),new string('t',120));
                form.ClientSize=new Size((int)(560*scale),(int)(320*scale));Verify(form,scenario+" compact");
                Find(form,"Status").Text="Installation complete. Source files verified: 235. RIP source preserved. Intro and CD music are not included; an original CD can supply music.";
                Verify(form,scenario+" folder completion");
                form.ClientSize=new Size((int)(760*scale),(int)(580*scale));Verify(form,scenario+" restored");
                mode.SelectedIndex=0;
                form.ClientSize=new Size((int)(560*scale),(int)(320*scale));Verify(form,scenario+" fix compact");
                Check(!Find(form,"Target").Parent.Visible,scenario+" fix has no copy destination");
                Check(Find(form,"Install").Text=="Install fix",scenario+" fix action");
                form.ClientSize=new Size((int)(760*scale),(int)(580*scale));Verify(form,scenario+" fix restored");
                form.CompleteInstallation(new InstallResult {OriginalFiles=235,Notice="Original game files are unchanged."});
                Verify(form,scenario+" finished");
                form.ClientSize=new Size((int)(560*scale),(int)(320*scale));Verify(form,scenario+" finished compact");
            }
        }
        foreach(bool folder in new[]{false,true}) {
            foreach(Keys key in new[]{Keys.Enter,Keys.Escape})using(var form=new SetupForm()) {
                // Exercise WinForms default/cancel routing with a real, transparent window.
                form.ShowInTaskbar=false;form.Opacity=0;
                ((ComboBox)Find(form,"Mode")).SelectedIndex=folder?0:1;
                form.Show();
                Check(Find(form,"Install").Visible,"ready exposes install action");
                Check(form.AcceptButton==Find(form,"Install"),"ready Enter installs");
                form.CompleteInstallation(new InstallResult {OriginalFiles=235,Notice="Original game files are unchanged.",ShortcutWarning="Shortcut could not be created."});
                Check(!Find(form,"Install").Visible && !Find(form,"Install").Enabled,"completion removes install action");
                foreach(string name in new[]{"Mode","Source","Target","SourceBrowse","TargetBrowse","Shortcut"})Check(!Find(form,name).Enabled,"completion locks "+name);
                Check(Find(form,"Close").Text=="Finish" && Find(form,"Close").Enabled,"completion offers Finish");
                Check(form.AcceptButton==Find(form,"Close") && form.CancelButton==Find(form,"Close"),"completion Enter and Escape finish");
                Check(((ProgressBar)Find(form,"Progress")).Value==100,"completion reports full progress");
                Check(Find(form,"Status").Text.Contains("235") && Find(form,"Status").Text.Contains("Original game files are unchanged."),"completion retains verification notice");
                Check(Find(form,"Details").Text.Contains("Shortcut could not be created."),"completion retains shortcut warning");
                ((ComboBox)Find(form,"Mode")).SelectedIndex=folder?1:0;
                Check(Find(form,"Introduction").Text.Contains("Choose Finish"),"mode changes cannot restore installation instructions");
                var task=(System.Threading.Tasks.Task)typeof(SetupForm).GetMethod("Install",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,null);
                Check(task.IsCompleted && !task.IsFaulted,"completed setup rejects another installation command");
                typeof(Form).GetMethod("ProcessDialogKey",BindingFlags.Instance|BindingFlags.NonPublic).Invoke(form,new object[]{key});
                Check(form.IsDisposed,"completion closes on "+key);
            }
        }
        Console.WriteLine(failures==0?"Installer layout at 100-300 percent and terminal completion actions: PASS":"Installer layout: FAIL");return failures==0?0:1;
    }
}
