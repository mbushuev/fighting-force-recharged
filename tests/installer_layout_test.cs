using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
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
        foreach(var control in Children(form)) {
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
                form.ClientSize=new Size((int)(760*scale),(int)(580*scale));Verify(form,scenario);
                Find(form,"Status").Text="Copying and verifying the original game files. The installation is still in progress; keep the source disc mounted until verification completes.";
                Find(form,"Source").Text=Path.Combine(Path.GetTempPath(),new string('s',120));
                Find(form,"Target").Text=Path.Combine(Path.GetTempPath(),new string('t',120));
                form.ClientSize=new Size((int)(560*scale),(int)(320*scale));Verify(form,scenario+" compact");
                Find(form,"Status").Text="Installation complete. Source files verified: 235. RIP source preserved. Intro and CD music are not included; an original CD can supply music.";
                Verify(form,scenario+" folder completion");
                form.ClientSize=new Size((int)(760*scale),(int)(580*scale));Verify(form,scenario+" restored");
            }
        }
        Console.WriteLine(failures==0?"Installer layout at 100-300 percent, compact and restored: PASS":"Installer layout: FAIL");return failures==0?0:1;
    }
}
