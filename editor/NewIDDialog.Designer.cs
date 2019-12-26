namespace editor
{
    partial class NewIDDialog
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
            this.IDSC = new System.Windows.Forms.NumericUpDown();
            this.OKBT = new System.Windows.Forms.Button();
            this.CancelBT = new System.Windows.Forms.Button();
            label1 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.IDSC)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(36, 26);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(18, 13);
            label1.TabIndex = 0;
            label1.Text = "ID";
            // 
            // IDSC
            // 
            this.IDSC.Location = new System.Drawing.Point(62, 24);
            this.IDSC.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.IDSC.Name = "IDSC";
            this.IDSC.Size = new System.Drawing.Size(120, 20);
            this.IDSC.TabIndex = 1;
            this.IDSC.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.IDSC.ValueChanged += new System.EventHandler(this.IDSC_ValueChanged);
            // 
            // OKBT
            // 
            this.OKBT.Location = new System.Drawing.Point(39, 59);
            this.OKBT.Name = "OKBT";
            this.OKBT.Size = new System.Drawing.Size(75, 23);
            this.OKBT.TabIndex = 2;
            this.OKBT.Text = "OK";
            this.OKBT.UseVisualStyleBackColor = true;
            this.OKBT.Click += new System.EventHandler(this.OKBT_Click);
            // 
            // CancelBT
            // 
            this.CancelBT.Location = new System.Drawing.Point(120, 59);
            this.CancelBT.Name = "CancelBT";
            this.CancelBT.Size = new System.Drawing.Size(75, 23);
            this.CancelBT.TabIndex = 3;
            this.CancelBT.Text = "Cancel";
            this.CancelBT.UseVisualStyleBackColor = true;
            this.CancelBT.Click += new System.EventHandler(this.CancelBT_Click);
            // 
            // NewIDDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(244, 101);
            this.Controls.Add(this.CancelBT);
            this.Controls.Add(this.OKBT);
            this.Controls.Add(this.IDSC);
            this.Controls.Add(label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Name = "NewIDDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "New ID";
            ((System.ComponentModel.ISupportInitialize)(this.IDSC)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.NumericUpDown IDSC;
        private System.Windows.Forms.Button OKBT;
        private System.Windows.Forms.Button CancelBT;
    }
}