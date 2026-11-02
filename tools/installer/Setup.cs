using System;
using System.Drawing;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace FightingForceSetup {
    static class Program {
        [STAThread] static int Main(string[] args) {
            if(args.Length>0)return CommandLine(args);
            Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new SetupForm());return 0;
        }
        static int CommandLine(string[] args) {
            try {
                if(args[0]=="--help") {Console.WriteLine("Fighting Force English disc setup and folder fix\n--apply --target <existing game folder> [--shortcut]\n--install --source <disc root> --target <new folder> [--shortcut]\n--verify --target <installed folder>\n--inspect-source --source <disc root or game folder>\n--media and --inspect-media remain accepted aliases. Supplied game files are never patched.");return 0;}
                string media=null,target=null;bool shortcut=false;
                for(int i=1;i<args.Length;++i) {
                    if((args[i]=="--media" || args[i]=="--source") && i+1<args.Length)media=args[++i];
                    else if(args[i]=="--target" && i+1<args.Length)target=args[++i];
                    else if(args[i]=="--shortcut")shortcut=true;
                    else throw new ArgumentException("Unknown or incomplete command-line option.");
                }
                var core=new InstallerCore((value,text)=>Console.WriteLine(value+"% "+text),CancellationToken.None);
                if(args[0]=="--verify" && target!=null) {Console.WriteLine("Verified source files: "+core.Verify(target));return 0;}
                if((args[0]=="--inspect-media" || args[0]=="--inspect-source") && media!=null) {
                    var source=core.InspectSource(media);
                    Console.WriteLine(source.Manifest.edition+"; source files: "+source.Manifest.files.Length+"; mode: "+(source.Folder?"folder":"disc"));
                    Console.WriteLine(source.Notice);return 0;
                }
                if((args[0]=="--install" && media!=null && target!=null) || (args[0]=="--apply" && target!=null && media==null)) {
                    var result=args[0]=="--apply"?core.Apply(target,shortcut):core.Install(media,target,shortcut);
                    Console.WriteLine("Installed and verified source files: "+result.OriginalFiles);
                    Console.WriteLine(result.Notice);
                    if(result.ShortcutWarning!=null)Console.WriteLine(result.ShortcutWarning);return 0;
                }
                throw new ArgumentException("Use --help for supported commands.");
            } catch(Exception e) {Console.Error.WriteLine(e.Message);return 1;}
        }
    }
    sealed class SetupForm:Form {
        readonly TextBox media=new TextBox(),target=new TextBox();
        readonly Button mediaBrowse=new Button(),targetBrowse=new Button(),install=new Button(),close=new Button();
        readonly CheckBox shortcut=new CheckBox();readonly ProgressBar progress=new ProgressBar();
        readonly ComboBox mode=new ComboBox();
        readonly Label status=new Label();readonly TextBox detail=new TextBox();
        readonly Label introduction;
        CancellationTokenSource cancellation;bool busy,completed;
        public SetupForm() {
            SuspendLayout();
            Text="Fighting Force Recharged - Setup";StartPosition=FormStartPosition.CenterScreen;
            Font=new Font("Segoe UI",10);
            ClientSize=new Size(760,580);MinimumSize=SizeFromClientSize(new Size(560,320));
            var frame=new TableLayoutPanel {Dock=DockStyle.Fill,Padding=new Padding(24),ColumnCount=1,RowCount=2};
            frame.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
            frame.RowStyles.Add(new RowStyle(SizeType.Percent,100));frame.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            var body=new Panel {Name="SetupBody",Dock=DockStyle.Fill,AutoScroll=true,Margin=Padding.Empty,TabIndex=0};
            var layout=new TableLayoutPanel {Name="SetupContent",Dock=DockStyle.Top,AutoSize=true,AutoSizeMode=AutoSizeMode.GrowAndShrink,ColumnCount=1,RowCount=12,Margin=Padding.Empty};
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));
            for(int i=0;i<layout.RowCount;++i)layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            layout.SuspendLayout();frame.SuspendLayout();
            var title=AddText(layout,"Title","Set up Fighting Force Recharged",0,new Padding(0,0,0,12));
            title.Font=new Font(Font,FontStyle.Bold);
            mode.Name="Mode";mode.DropDownStyle=ComboBoxStyle.DropDownList;mode.Dock=DockStyle.Top;mode.Margin=new Padding(0,0,0,12);
            mode.Items.AddRange(new object[]{"Add fix to existing game folder","Install game from original disc"});mode.SelectedIndex=0;layout.Controls.Add(mode,0,1);
            introduction=AddText(layout,"Introduction","",2,new Padding(0,0,0,18));
            var sourceLabel=AddText(layout,"SourceLabel","Game folder",3,new Padding(0,0,0,4));
            AddPath(layout,media,mediaBrowse,"Source",4);
            var targetLabel=AddText(layout,"TargetLabel","Install folder",5,new Padding(0,10,0,4));
            AddPath(layout,target,targetBrowse,"Target",6);
            target.Text=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Programs","Fighting Force");
            var destinationNote=AddText(layout,"DestinationNote","",7,new Padding(0,6,0,10));
            shortcut.Name="Shortcut";shortcut.Text="Create a desktop shortcut";shortcut.Checked=true;shortcut.AutoSize=true;shortcut.Margin=Padding.Empty;
            layout.Controls.Add(shortcut,0,8);
            status.Name="Status";status.AutoSize=true;status.Dock=DockStyle.Top;status.Text="Ready. Select the original disc or a supported game folder.";status.Margin=new Padding(0,14,0,8);
            layout.Controls.Add(status,0,9);
            progress.Name="Progress";progress.Dock=DockStyle.Top;progress.Height=18;progress.Margin=new Padding(0,0,0,8);layout.Controls.Add(progress,0,10);
            detail.Name="Details";detail.Multiline=true;detail.ReadOnly=true;detail.ScrollBars=ScrollBars.Vertical;detail.Dock=DockStyle.Top;detail.Height=100;detail.BackColor=SystemColors.Window;detail.Margin=Padding.Empty;
            layout.Controls.Add(detail,0,11);
            // Bound label measurement to the viewport, including when a vertical scrollbar appears.
            layout.SizeChanged+=delegate {foreach(Control control in layout.Controls)if(control is Label)control.MaximumSize=new Size(Math.Max(1,layout.ClientSize.Width-control.Margin.Horizontal),0);};
            var actions=new FlowLayoutPanel {Name="Actions",AutoSize=true,AutoSizeMode=AutoSizeMode.GrowAndShrink,FlowDirection=FlowDirection.RightToLeft,WrapContents=false,Dock=DockStyle.Fill,Margin=new Padding(0,16,0,0),TabIndex=1};
            ConfigureButton(close,"Close","Close");ConfigureButton(install,"Install","Install");
            close.Margin=Padding.Empty;install.Margin=new Padding(0,0,8,0);actions.Controls.Add(close);actions.Controls.Add(install);
            body.Controls.Add(layout);frame.Controls.Add(body,0,0);frame.Controls.Add(actions,0,1);Controls.Add(frame);AcceptButton=install;
            AutoScaleDimensions=new SizeF(96,96);AutoScaleMode=AutoScaleMode.Dpi;
            layout.ResumeLayout(false);frame.ResumeLayout(false);ResumeLayout(false);
            Action updateMode=delegate {
                if(completed)return;
                bool folder=mode.SelectedIndex==0;
                introduction.Text=folder?"Adds the fix directly beside fforce.exe in the selected game folder. Verifies the supported English retail or RIP files without copying or modifying them.":"Installs the supported English original disc into a new folder, verifies its files, then adds the fix. No game files are patched.";
                sourceLabel.Text=folder?"Game folder":"Original disc";
                targetLabel.Visible=target.Parent.Visible=!folder;
                destinationNote.Text=folder?"Existing settings and saves are kept. The RIP has no intro or CD music; an original CD can supply music.":"Choose a new folder. Existing installations are never overwritten. Keep the original disc mounted to play.";
                install.Text=folder?"Install fix":"Install game";
            };
            mode.SelectedIndexChanged+=delegate {updateMode();};updateMode();
            media.TextChanged+=delegate {
                if(busy)return;
                try {
                    if(File.Exists(Path.Combine(media.Text.Trim(),"fforce.exe")))mode.SelectedIndex=0;
                    else if(File.Exists(Path.Combine(media.Text.Trim(),"disk1","DATA1.CAB")))mode.SelectedIndex=1;
                }catch(ArgumentException) {}catch(NotSupportedException) {}
            };
            mediaBrowse.Click+=delegate {using(var pick=new FolderBrowserDialog {Description="Select the original disc root or the supported game folder containing fforce.exe and Wads."})if(pick.ShowDialog(this)==DialogResult.OK)media.Text=pick.SelectedPath;};
            targetBrowse.Click+=delegate {using(var pick=new FolderBrowserDialog {Description="Select a parent folder. A new Fighting Force folder will be created inside it."})if(pick.ShowDialog(this)==DialogResult.OK)target.Text=Path.Combine(pick.SelectedPath,"Fighting Force");};
            close.Click+=delegate {if(busy) {cancellation.Cancel();close.Enabled=false;}else Close();};
            FormClosing+=delegate(object sender,FormClosingEventArgs e) {if(busy) {cancellation.Cancel();e.Cancel=true;}};
            install.Click+=async delegate {await Install();};
            Shown+=delegate {try {if(media.Text.Length==0)media.Text=new InstallerCore(null,CancellationToken.None).DetectMedia();}catch(Exception e) {status.Text=e.Message;}};
        }
        static Label AddText(TableLayoutPanel layout,string name,string text,int row,Padding margin) {
            var label=new Label {Name=name,Text=text,AutoSize=true,Dock=DockStyle.Top,Margin=margin};
            layout.Controls.Add(label,0,row);return label;
        }
        static void ConfigureButton(Button button,string name,string text) {
            button.Name=name;button.Text=text;button.AutoSize=true;button.AutoSizeMode=AutoSizeMode.GrowAndShrink;
            button.MinimumSize=new Size(96,0);button.Padding=new Padding(12,5,12,5);
        }
        static void AddPath(TableLayoutPanel layout,TextBox path,Button browse,string name,int row) {
            var field=new TableLayoutPanel {AutoSize=true,AutoSizeMode=AutoSizeMode.GrowAndShrink,Dock=DockStyle.Top,ColumnCount=2,RowCount=1,Margin=Padding.Empty};
            field.ColumnStyles.Add(new ColumnStyle(SizeType.Percent,100));field.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));field.RowStyles.Add(new RowStyle(SizeType.AutoSize));
            path.Name=name;path.Dock=DockStyle.Fill;path.Anchor=AnchorStyles.Left|AnchorStyles.Right;path.Margin=new Padding(0,0,8,0);
            ConfigureButton(browse,name+"Browse","Browse...");browse.Margin=Padding.Empty;
            field.Controls.Add(path,0,0);field.Controls.Add(browse,1,0);layout.Controls.Add(field,0,row);
        }
        protected override void OnLoad(EventArgs e) {
            base.OnLoad(e);
            // Keep the action row visible on small desktops; the content can scroll vertically.
            var area=Screen.FromControl(this).WorkingArea;
            MinimumSize=new Size(Math.Min(MinimumSize.Width,area.Width),Math.Min(MinimumSize.Height,area.Height));
            Size=new Size(Math.Min(Width,area.Width),Math.Min(Height,area.Height));
        }
        async Task Install() {
            if(busy || completed)return;
            string source=media.Text.Trim(),destination=target.Text.Trim();bool wantShortcut=shortcut.Checked,folder=mode.SelectedIndex==0;
            if(source.Length==0 || (!folder && destination.Length==0)) {MessageBox.Show(this,folder?"Select the game folder containing fforce.exe.":"Select the original disc and a new installation folder.",Text);return;}
            busy=true;cancellation=new CancellationTokenSource();SetInputs(false);AcceptButton=null;close.Text="Cancel";detail.Clear();progress.Value=0;
            InstallResult result=null;
            try {
                // The per-user Programs parent may not exist on a fresh profile.
                string defaultParent=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"Programs");
                if(!folder && String.Equals(Path.GetDirectoryName(Path.GetFullPath(destination)),defaultParent,StringComparison.OrdinalIgnoreCase))Directory.CreateDirectory(defaultParent);
                var core=new InstallerCore((value,text)=>BeginInvoke(new Action(()=>{if(completed)return;progress.Value=value;status.Text=text;detail.AppendText(text+Environment.NewLine);})),cancellation.Token);
                result=await Task.Run(()=>folder?core.Apply(source,wantShortcut):core.Install(source,destination,wantShortcut));
            } catch(OperationCanceledException) {status.Text=folder?"Cancelled. Compatibility changes were rolled back.":"Cancelled. The incomplete copy was removed.";SetInputs(true);}
            catch(Exception e) {status.Text="Installation did not complete.";detail.AppendText(e.Message+Environment.NewLine);SetInputs(true);MessageBox.Show(this,e.Message,Text,MessageBoxButtons.OK,MessageBoxIcon.Error);}
            finally {busy=false;close.Text="Close";close.Enabled=true;AcceptButton=install;cancellation.Dispose();cancellation=null;}
            if(result!=null)CompleteInstallation(result);
        }
        internal void CompleteInstallation(InstallResult result) {
            completed=true;SetInputs(false);install.Visible=false;progress.Value=100;
            status.Text="Installation complete. Source files verified: "+result.OriginalFiles+". "+result.Notice;
            if(result.ShortcutWarning!=null)detail.AppendText(result.ShortcutWarning+Environment.NewLine);
            introduction.Text="Fighting Force Recharged is installed. Choose Finish, then start fforce.exe from the game folder.";
            close.Text="Finish";close.Enabled=true;AcceptButton=close;CancelButton=close;close.Select();
        }
        void SetInputs(bool enabled) {mode.Enabled=media.Enabled=target.Enabled=mediaBrowse.Enabled=targetBrowse.Enabled=shortcut.Enabled=install.Enabled=enabled;}
    }
}
