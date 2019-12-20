namespace editor
{
    partial class NewMapDialog
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
            this.label1 = new System.Windows.Forms.Label();
            this.NameTB = new System.Windows.Forms.TextBox();
            this.IDSC = new System.Windows.Forms.NumericUpDown();
            this.label2 = new System.Windows.Forms.Label();
            this.OKBT = new System.Windows.Forms.Button();
            this.CancelBT = new System.Windows.Forms.Button();
            this.WSC = new System.Windows.Forms.NumericUpDown();
            this.HSC = new System.Windows.Forms.NumericUpDown();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.IDSC)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.WSC)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.HSC)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 28);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(35, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Name";
            // 
            // NameTB
            // 
            this.NameTB.Location = new System.Drawing.Point(53, 25);
            this.NameTB.Name = "NameTB";
            this.NameTB.Size = new System.Drawing.Size(239, 20);
            this.NameTB.TabIndex = 1;
            this.NameTB.TextChanged += new System.EventHandler(this.NameTB_TextChanged);
            // 
            // IDSC
            // 
            this.IDSC.Location = new System.Drawing.Point(53, 51);
            this.IDSC.Maximum = new decimal(new int[] {
            511,
            0,
            0,
            0});
            this.IDSC.Name = "IDSC";
            this.IDSC.Size = new System.Drawing.Size(120, 20);
            this.IDSC.TabIndex = 2;
            this.IDSC.ValueChanged += new System.EventHandler(this.IDSC_ValueChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 53);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(18, 13);
            this.label2.TabIndex = 3;
            this.label2.Text = "ID";
            // 
            // OKBT
            // 
            this.OKBT.Location = new System.Drawing.Point(80, 116);
            this.OKBT.Name = "OKBT";
            this.OKBT.Size = new System.Drawing.Size(75, 23);
            this.OKBT.TabIndex = 4;
            this.OKBT.Text = "OK";
            this.OKBT.UseVisualStyleBackColor = true;
            this.OKBT.Click += new System.EventHandler(this.OKBT_Click);
            // 
            // CancelBT
            // 
            this.CancelBT.Location = new System.Drawing.Point(161, 116);
            this.CancelBT.Name = "CancelBT";
            this.CancelBT.Size = new System.Drawing.Size(75, 23);
            this.CancelBT.TabIndex = 5;
            this.CancelBT.Text = "Cancel";
            this.CancelBT.UseVisualStyleBackColor = true;
            this.CancelBT.Click += new System.EventHandler(this.CancelBT_Click);
            // 
            // WSC
            // 
            this.WSC.Location = new System.Drawing.Point(53, 77);
            this.WSC.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.WSC.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.WSC.Name = "WSC";
            this.WSC.Size = new System.Drawing.Size(58, 20);
            this.WSC.TabIndex = 6;
            this.WSC.Value = new decimal(new int[] {
            16,
            0,
            0,
            0});
            this.WSC.ValueChanged += new System.EventHandler(this.WSC_ValueChanged);
            // 
            // HSC
            // 
            this.HSC.Location = new System.Drawing.Point(161, 77);
            this.HSC.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.HSC.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
            this.HSC.Name = "HSC";
            this.HSC.Size = new System.Drawing.Size(58, 20);
            this.HSC.TabIndex = 7;
            this.HSC.Value = new decimal(new int[] {
            16,
            0,
            0,
            0});
            this.HSC.ValueChanged += new System.EventHandler(this.HSC_ValueChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 79);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(35, 13);
            this.label3.TabIndex = 8;
            this.label3.Text = "Width";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(117, 79);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(38, 13);
            this.label4.TabIndex = 9;
            this.label4.Text = "Height";
            // 
            // NewMapDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(317, 158);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.HSC);
            this.Controls.Add(this.WSC);
            this.Controls.Add(this.CancelBT);
            this.Controls.Add(this.OKBT);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.IDSC);
            this.Controls.Add(this.NameTB);
            this.Controls.Add(this.label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.Name = "NewMapDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "New Map";
            ((System.ComponentModel.ISupportInitialize)(this.IDSC)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.WSC)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.HSC)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox NameTB;
        private System.Windows.Forms.NumericUpDown IDSC;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button OKBT;
        private System.Windows.Forms.Button CancelBT;
        private System.Windows.Forms.NumericUpDown WSC;
        private System.Windows.Forms.NumericUpDown HSC;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
    }
}