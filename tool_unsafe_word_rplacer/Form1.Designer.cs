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
            this.tbPreview = new System.Windows.Forms.RichTextBox();
            this.tbDirs = new System.Windows.Forms.RichTextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.tbSafes = new System.Windows.Forms.RichTextBox();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
            this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
            this.tbExts = new System.Windows.Forms.RichTextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.menuRoot = new System.Windows.Forms.MenuStrip();
            this.mHelp = new System.Windows.Forms.ToolStripMenuItem();
            this.mAbout = new System.Windows.Forms.ToolStripMenuItem();
            this.mRandomSelected = new System.Windows.Forms.ToolStripMenuItem();
            this.tbIgnores = new System.Windows.Forms.RichTextBox();
            this.label8 = new System.Windows.Forms.Label();
            this.dgWords = new System.Windows.Forms.DataGridView();
            this.From = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.To = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.Nums = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.mScan = new System.Windows.Forms.ToolStripMenuItem();
            this.mImport = new System.Windows.Forms.ToolStripMenuItem();
            this.mExport = new System.Windows.Forms.ToolStripMenuItem();
            this.mRandomAll = new System.Windows.Forms.ToolStripMenuItem();
            this.mMoveAllWordsToSafes = new System.Windows.Forms.ToolStripMenuItem();
            this.menuRoot.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dgWords)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(10, 25);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(209, 12);
            this.label1.TabIndex = 1;
            this.label1.Text = "directories: ( \\* mean recursive )";
            // 
            // tbPreview
            // 
            this.tbPreview.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbPreview.Location = new System.Drawing.Point(520, 186);
            this.tbPreview.Name = "tbPreview";
            this.tbPreview.ReadOnly = true;
            this.tbPreview.Size = new System.Drawing.Size(452, 363);
            this.tbPreview.TabIndex = 5;
            this.tbPreview.Text = "";
            // 
            // tbDirs
            // 
            this.tbDirs.Location = new System.Drawing.Point(12, 40);
            this.tbDirs.Name = "tbDirs";
            this.tbDirs.Size = new System.Drawing.Size(354, 116);
            this.tbDirs.TabIndex = 0;
            this.tbDirs.Text = "C:\\Users\\xx\\Documents\\adxe1\nC:\\Users\\xx\\Documents\\adxe2\\*\nC:\\Users\\xx\\Documents\\a" +
    "dxe3\\*";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(619, 25);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(41, 12);
            this.label2.TabIndex = 7;
            this.label2.Text = "safes:";
            // 
            // tbSafes
            // 
            this.tbSafes.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.tbSafes.Location = new System.Drawing.Point(621, 40);
            this.tbSafes.Name = "tbSafes";
            this.tbSafes.Size = new System.Drawing.Size(351, 116);
            this.tbSafes.TabIndex = 3;
            this.tbSafes.Text = "";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(10, 168);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(59, 12);
            this.label3.TabIndex = 9;
            this.label3.Text = "replaces:";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(518, 168);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(179, 12);
            this.label4.TabIndex = 10;
            this.label4.Text = "selected row replace preview:";
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
            this.tbExts.Size = new System.Drawing.Size(73, 116);
            this.tbExts.TabIndex = 1;
            this.tbExts.Text = ".cpp\n.lua\n.h\n.inc\n.cs";
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
            this.mImport,
            this.mExport,
            this.mScan,
            this.mRandomSelected,
            this.mRandomAll,
            this.mMoveAllWordsToSafes,
            this.mHelp});
            this.menuRoot.Location = new System.Drawing.Point(0, 0);
            this.menuRoot.Name = "menuRoot";
            this.menuRoot.Size = new System.Drawing.Size(984, 25);
            this.menuRoot.TabIndex = 18;
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
            // mRandomSelected
            // 
            this.mRandomSelected.Name = "mRandomSelected";
            this.mRandomSelected.Size = new System.Drawing.Size(122, 21);
            this.mRandomSelected.Text = "&Random Selected";
            this.mRandomSelected.Click += new System.EventHandler(this.mRandomSelected_Click);
            // 
            // tbIgnores
            // 
            this.tbIgnores.Location = new System.Drawing.Point(452, 40);
            this.tbIgnores.Name = "tbIgnores";
            this.tbIgnores.Size = new System.Drawing.Size(163, 116);
            this.tbIgnores.TabIndex = 2;
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
            // dgWords
            // 
            this.dgWords.AllowUserToOrderColumns = true;
            this.dgWords.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
            this.dgWords.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dgWords.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.From,
            this.To,
            this.Nums});
            this.dgWords.Location = new System.Drawing.Point(12, 186);
            this.dgWords.Name = "dgWords";
            this.dgWords.RowTemplate.Height = 23;
            this.dgWords.Size = new System.Drawing.Size(502, 363);
            this.dgWords.TabIndex = 4;
            this.dgWords.CellStateChanged += new System.Windows.Forms.DataGridViewCellStateChangedEventHandler(this.dgWords_CellStateChanged);
            this.dgWords.CellValueChanged += new System.Windows.Forms.DataGridViewCellEventHandler(this.dgWords_CellValueChanged);
            this.dgWords.RowStateChanged += new System.Windows.Forms.DataGridViewRowStateChangedEventHandler(this.dgWords_RowStateChanged);
            // 
            // From
            // 
            this.From.DataPropertyName = "From";
            this.From.FillWeight = 200F;
            this.From.Frozen = true;
            this.From.HeaderText = "From";
            this.From.MaxInputLength = 64;
            this.From.Name = "From";
            this.From.ReadOnly = true;
            this.From.Width = 200;
            // 
            // To
            // 
            this.To.DataPropertyName = "To";
            this.To.FillWeight = 200F;
            this.To.Frozen = true;
            this.To.HeaderText = "To";
            this.To.MaxInputLength = 64;
            this.To.Name = "To";
            this.To.Width = 200;
            // 
            // Nums
            // 
            this.Nums.DataPropertyName = "Nums";
            this.Nums.FillWeight = 35F;
            this.Nums.Frozen = true;
            this.Nums.HeaderText = "Nums";
            this.Nums.MaxInputLength = 64;
            this.Nums.Name = "Nums";
            this.Nums.ReadOnly = true;
            this.Nums.Width = 35;
            // 
            // mScan
            // 
            this.mScan.Name = "mScan";
            this.mScan.Size = new System.Drawing.Size(82, 21);
            this.mScan.Text = "&Scan / Run";
            this.mScan.Click += new System.EventHandler(this.mScan_Click);
            // 
            // mImport
            // 
            this.mImport.Name = "mImport";
            this.mImport.Size = new System.Drawing.Size(60, 21);
            this.mImport.Text = "&Import";
            this.mImport.Click += new System.EventHandler(this.mImport_Click);
            // 
            // mExport
            // 
            this.mExport.Name = "mExport";
            this.mExport.Size = new System.Drawing.Size(58, 21);
            this.mExport.Text = "&Export";
            this.mExport.Click += new System.EventHandler(this.mExport_Click);
            // 
            // mRandomAll
            // 
            this.mRandomAll.Name = "mRandomAll";
            this.mRandomAll.Size = new System.Drawing.Size(87, 21);
            this.mRandomAll.Text = "Random &All";
            this.mRandomAll.Click += new System.EventHandler(this.mRandomAll_Click);
            // 
            // mMoveAllWordsToSafes
            // 
            this.mMoveAllWordsToSafes.Name = "mMoveAllWordsToSafes";
            this.mMoveAllWordsToSafes.Size = new System.Drawing.Size(160, 21);
            this.mMoveAllWordsToSafes.Text = "&Move all words to safes";
            this.mMoveAllWordsToSafes.Click += new System.EventHandler(this.mMoveAllWordsToSafes_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(984, 561);
            this.Controls.Add(this.dgWords);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.tbIgnores);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.tbExts);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.tbSafes);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.tbDirs);
            this.Controls.Add(this.tbPreview);
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
            ((System.ComponentModel.ISupportInitialize)(this.dgWords)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.RichTextBox tbPreview;
        private System.Windows.Forms.RichTextBox tbDirs;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.RichTextBox tbSafes;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.OpenFileDialog openFileDialog1;
        private System.Windows.Forms.SaveFileDialog saveFileDialog1;
        private System.Windows.Forms.RichTextBox tbExts;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.MenuStrip menuRoot;
        private System.Windows.Forms.ToolStripMenuItem mHelp;
        private System.Windows.Forms.ToolStripMenuItem mAbout;
        private System.Windows.Forms.RichTextBox tbIgnores;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.DataGridView dgWords;
        private System.Windows.Forms.ToolStripMenuItem mRandomSelected;
        private System.Windows.Forms.DataGridViewTextBoxColumn From;
        private System.Windows.Forms.DataGridViewTextBoxColumn To;
        private System.Windows.Forms.DataGridViewTextBoxColumn Nums;
        private System.Windows.Forms.ToolStripMenuItem mScan;
        private System.Windows.Forms.ToolStripMenuItem mImport;
        private System.Windows.Forms.ToolStripMenuItem mExport;
        private System.Windows.Forms.ToolStripMenuItem mRandomAll;
        private System.Windows.Forms.ToolStripMenuItem mMoveAllWordsToSafes;
    }
}

