namespace UnsafeWordReplacer
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.label1 = new System.Windows.Forms.Label();
            this.tvWords = new System.Windows.Forms.TreeView();
            this.tbPreview = new System.Windows.Forms.RichTextBox();
            this.tbDirs = new System.Windows.Forms.RichTextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.tbSafes = new System.Windows.Forms.RichTextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.tbExts = new System.Windows.Forms.RichTextBox();
            this.label5 = new System.Windows.Forms.Label();
            this.tbOriginal = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.tbReplaceTo = new System.Windows.Forms.TextBox();
            this.bRestore = new System.Windows.Forms.Button();
            this.label7 = new System.Windows.Forms.Label();
            this.menuRoot = new System.Windows.Forms.MenuStrip();
            this.mFile = new System.Windows.Forms.ToolStripMenuItem();
            this.mImport = new System.Windows.Forms.ToolStripMenuItem();
            this.mExport = new System.Windows.Forms.ToolStripMenuItem();
            this.mSave = new System.Windows.Forms.ToolStripMenuItem();
            this.mQuit = new System.Windows.Forms.ToolStripMenuItem();
            this.editToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mScan = new System.Windows.Forms.ToolStripMenuItem();
            this.mRandomAll = new System.Windows.Forms.ToolStripMenuItem();
            this.mHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.mAbout = new System.Windows.Forms.ToolStripMenuItem();
            this.tbIgnores = new System.Windows.Forms.RichTextBox();
            this.label8 = new System.Windows.Forms.Label();
            this.menuRoot.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 25);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(209, 12);
            this.label1.TabIndex = 1;
            this.label1.Text = "directories: ( \\* mean recursive )";
            // 
            // tvWords
            // 
            this.tvWords.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.tvWords.Location = new System.Drawing.Point(14, 174);
            this.tvWords.Name = "tvWords";
            this.tvWords.Size = new System.Drawing.Size(235, 375);
            this.tvWords.TabIndex = 3;
            this.tvWords.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.tvWords_AfterSelect);
            this.tvWords.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.tvWords_KeyPress);
            // 
            // tbPreview
            // 
            this.tbPreview.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbPreview.Location = new System.Drawing.Point(316, 186);
            this.tbPreview.Name = "tbPreview";
            this.tbPreview.ReadOnly = true;
            this.tbPreview.Size = new System.Drawing.Size(656, 363);
            this.tbPreview.TabIndex = 8;
            this.tbPreview.Text = "";
            // 
            // tbDirs
            // 
            this.tbDirs.Location = new System.Drawing.Point(14, 40);
            this.tbDirs.Name = "tbDirs";
            this.tbDirs.Size = new System.Drawing.Size(352, 116);
            this.tbDirs.TabIndex = 0;
            this.tbDirs.Text = "C:\\Users\\xx\\Documents\\adxe1\nC:\\Users\\xx\\Documents\\adxe2\\*\nC:\\Users\\xx\\Documents\\a" +
    "dxe3\\*";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(641, 25);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(71, 12);
            this.label2.TabIndex = 7;
            this.label2.Text = "safe words:";
            // 
            // tbSafes
            // 
            this.tbSafes.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbSafes.Location = new System.Drawing.Point(621, 40);
            this.tbSafes.Name = "tbSafes";
            this.tbSafes.Size = new System.Drawing.Size(351, 111);
            this.tbSafes.TabIndex = 2;
            this.tbSafes.Text = "if then goto self this var auto while for do class struct enum";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 159);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(209, 12);
            this.label3.TabIndex = 9;
            this.label3.Text = "words: ( SPACE ->safe R ->random )";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(261, 186);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(53, 12);
            this.label4.TabIndex = 10;
            this.label4.Text = "preview:";
            // 
            // openFileDialog1
            // 
            this.openFileDialog1.Filter = "Unsafe Word Replacer Config Files|*.uwr";
            // 
            // saveFileDialog1
            // 
            this.saveFileDialog1.Filter = "Unsafe Word Replacer Config Files|*.uwr";
            // 
            // tbExts
            // 
            this.tbExts.Location = new System.Drawing.Point(373, 40);
            this.tbExts.Name = "tbExts";
            this.tbExts.Size = new System.Drawing.Size(73, 113);
            this.tbExts.TabIndex = 1;
            this.tbExts.Text = ".cpp\n.lua\n.h\n.inc\n.cs";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(255, 162);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(59, 12);
            this.label5.TabIndex = 13;
            this.label5.Text = "original:";
            // 
            // tbOriginal
            // 
            this.tbOriginal.Location = new System.Drawing.Point(316, 159);
            this.tbOriginal.Name = "tbOriginal";
            this.tbOriginal.ReadOnly = true;
            this.tbOriginal.Size = new System.Drawing.Size(213, 21);
            this.tbOriginal.TabIndex = 4;
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(544, 162);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(71, 12);
            this.label6.TabIndex = 15;
            this.label6.Text = "replace to:";
            // 
            // tbReplaceTo
            // 
            this.tbReplaceTo.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbReplaceTo.Location = new System.Drawing.Point(621, 159);
            this.tbReplaceTo.Name = "tbReplaceTo";
            this.tbReplaceTo.Size = new System.Drawing.Size(282, 21);
            this.tbReplaceTo.TabIndex = 5;
            this.tbReplaceTo.TextChanged += new System.EventHandler(this.tbReplaceTo_TextChanged);
            // 
            // bRestore
            // 
            this.bRestore.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.bRestore.Location = new System.Drawing.Point(909, 157);
            this.bRestore.Name = "bRestore";
            this.bRestore.Size = new System.Drawing.Size(63, 23);
            this.bRestore.TabIndex = 6;
            this.bRestore.Text = "&Restore";
            this.bRestore.UseVisualStyleBackColor = true;
            this.bRestore.Click += new System.EventHandler(this.bRestore_Click);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(371, 25);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(35, 12);
            this.label7.TabIndex = 17;
            this.label7.Text = "exts:";
            // 
            // menuRoot
            // 
            this.menuRoot.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mFile,
            this.editToolStripMenuItem,
            this.mHelp});
            this.menuRoot.Location = new System.Drawing.Point(0, 0);
            this.menuRoot.Name = "menuRoot";
            this.menuRoot.Size = new System.Drawing.Size(984, 25);
            this.menuRoot.TabIndex = 18;
            // 
            // mFile
            // 
            this.mFile.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mImport,
            this.mExport,
            this.mSave,
            this.mQuit});
            this.mFile.Name = "mFile";
            this.mFile.Size = new System.Drawing.Size(39, 21);
            this.mFile.Text = "&File";
            // 
            // mImport
            // 
            this.mImport.Name = "mImport";
            this.mImport.Size = new System.Drawing.Size(116, 22);
            this.mImport.Text = "&Import";
            this.mImport.Click += new System.EventHandler(this.mImport_Click);
            // 
            // mExport
            // 
            this.mExport.Name = "mExport";
            this.mExport.Size = new System.Drawing.Size(116, 22);
            this.mExport.Text = "&Export";
            this.mExport.Click += new System.EventHandler(this.mExport_Click);
            // 
            // mSave
            // 
            this.mSave.Name = "mSave";
            this.mSave.Size = new System.Drawing.Size(116, 22);
            this.mSave.Text = "&Save";
            this.mSave.Click += new System.EventHandler(this.mSave_Click);
            // 
            // mQuit
            // 
            this.mQuit.Name = "mQuit";
            this.mQuit.Size = new System.Drawing.Size(116, 22);
            this.mQuit.Text = "&Quit";
            this.mQuit.Click += new System.EventHandler(this.mQuit_Click);
            // 
            // editToolStripMenuItem
            // 
            this.editToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mScan,
            this.mRandomAll});
            this.editToolStripMenuItem.Name = "editToolStripMenuItem";
            this.editToolStripMenuItem.Size = new System.Drawing.Size(42, 21);
            this.editToolStripMenuItem.Text = "&Edit";
            // 
            // mScan
            // 
            this.mScan.Name = "mScan";
            this.mScan.Size = new System.Drawing.Size(180, 22);
            this.mScan.Text = "&Scan";
            this.mScan.Click += new System.EventHandler(this.mScan_Click);
            // 
            // mRandomAll
            // 
            this.mRandomAll.Name = "mRandomAll";
            this.mRandomAll.Size = new System.Drawing.Size(180, 22);
            this.mRandomAll.Text = "Random &All";
            this.mRandomAll.Click += new System.EventHandler(this.mRandomAll_Click);
            // 
            // mHelp
            // 
            this.mHelp.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mAbout});
            this.mHelp.Name = "mHelp";
            this.mHelp.Size = new System.Drawing.Size(47, 21);
            this.mHelp.Text = "&Help";
            // 
            // mAbout
            // 
            this.mAbout.Name = "mAbout";
            this.mAbout.Size = new System.Drawing.Size(111, 22);
            this.mAbout.Text = "&About";
            this.mAbout.Click += new System.EventHandler(this.mAbout_Click);
            // 
            // tbIgnores
            // 
            this.tbIgnores.Location = new System.Drawing.Point(452, 40);
            this.tbIgnores.Name = "tbIgnores";
            this.tbIgnores.Size = new System.Drawing.Size(163, 113);
            this.tbIgnores.TabIndex = 19;
            this.tbIgnores.Text = "cmake-build-debug\ncmake-build-release\n.idea\n.vs\n.settings\n.cxx\nout\nbin\nobj\nx64\nbu" +
    "ild\nDebug\nRelease";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(450, 25);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(131, 12);
            this.label8.TabIndex = 20;
            this.label8.Text = "ignores: ( git like )";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(984, 561);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.tbIgnores);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.bRestore);
            this.Controls.Add(this.tbReplaceTo);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.tbOriginal);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.tbExts);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.tbSafes);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.tbDirs);
            this.Controls.Add(this.tbPreview);
            this.Controls.Add(this.tvWords);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.menuRoot);
            this.MainMenuStrip = this.menuRoot;
            this.Margin = new System.Windows.Forms.Padding(2);
            this.MinimumSize = new System.Drawing.Size(1000, 600);
            this.Name = "Form1";
            this.Text = "Unsafe Word Replacer";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.menuRoot.ResumeLayout(false);
            this.menuRoot.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TreeView tvWords;
        private System.Windows.Forms.RichTextBox tbPreview;
        private System.Windows.Forms.RichTextBox tbDirs;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.RichTextBox tbSafes;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.RichTextBox tbExts;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox tbOriginal;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox tbReplaceTo;
        private System.Windows.Forms.Button bRestore;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.MenuStrip menuRoot;
        private System.Windows.Forms.ToolStripMenuItem mFile;
        private System.Windows.Forms.ToolStripMenuItem mImport;
        private System.Windows.Forms.ToolStripMenuItem mExport;
        private System.Windows.Forms.ToolStripMenuItem mSave;
        private System.Windows.Forms.ToolStripMenuItem mQuit;
        private System.Windows.Forms.ToolStripMenuItem mHelp;
        private System.Windows.Forms.ToolStripMenuItem mAbout;
        private System.Windows.Forms.ToolStripMenuItem editToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem mScan;
        private System.Windows.Forms.ToolStripMenuItem mRandomAll;
        private System.Windows.Forms.RichTextBox tbIgnores;
        private System.Windows.Forms.Label label8;
    }
}

