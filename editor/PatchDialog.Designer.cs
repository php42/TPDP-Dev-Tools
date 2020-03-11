namespace editor
{
    partial class PatchDialog
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
            if(disposing && (components != null))
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
            System.Windows.Forms.Label label1;
            this.DirTB = new System.Windows.Forms.TextBox();
            this.BrowseBT = new System.Windows.Forms.Button();
            this.GenerateBT = new System.Windows.Forms.Button();
            this.BrowseDialog = new System.Windows.Forms.FolderBrowserDialog();
            this.SaveDialog = new System.Windows.Forms.SaveFileDialog();
            label1 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(81, 16);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(277, 13);
            label1.TabIndex = 0;
            label1.Text = "Select an UNMODIFIED game folder to compare against.";
            // 
            // DirTB
            // 
            this.DirTB.Location = new System.Drawing.Point(12, 36);
            this.DirTB.Name = "DirTB";
            this.DirTB.Size = new System.Drawing.Size(334, 20);
            this.DirTB.TabIndex = 1;
            // 
            // BrowseBT
            // 
            this.BrowseBT.Location = new System.Drawing.Point(352, 34);
            this.BrowseBT.Name = "BrowseBT";
            this.BrowseBT.Size = new System.Drawing.Size(75, 23);
            this.BrowseBT.TabIndex = 2;
            this.BrowseBT.Text = "Browse";
            this.BrowseBT.UseVisualStyleBackColor = true;
            this.BrowseBT.Click += new System.EventHandler(this.BrowseBT_Click);
            // 
            // GenerateBT
            // 
            this.GenerateBT.Location = new System.Drawing.Point(182, 73);
            this.GenerateBT.Name = "GenerateBT";
            this.GenerateBT.Size = new System.Drawing.Size(75, 23);
            this.GenerateBT.TabIndex = 3;
            this.GenerateBT.Text = "Generate";
            this.GenerateBT.UseVisualStyleBackColor = true;
            this.GenerateBT.Click += new System.EventHandler(this.GenerateBT_Click);
            // 
            // BrowseDialog
            // 
            this.BrowseDialog.Description = "Select Game Folder";
            // 
            // SaveDialog
            // 
            this.SaveDialog.DefaultExt = "bin";
            this.SaveDialog.FileName = "patch.bin";
            this.SaveDialog.Title = "Save Patch File";
            // 
            // PatchDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(439, 112);
            this.Controls.Add(this.GenerateBT);
            this.Controls.Add(this.BrowseBT);
            this.Controls.Add(this.DirTB);
            this.Controls.Add(label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Name = "PatchDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Create Patch";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox DirTB;
        private System.Windows.Forms.Button BrowseBT;
        private System.Windows.Forms.Button GenerateBT;
        private System.Windows.Forms.FolderBrowserDialog BrowseDialog;
        private System.Windows.Forms.SaveFileDialog SaveDialog;
    }
}