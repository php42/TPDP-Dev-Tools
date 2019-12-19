namespace editor
{
    partial class ShiftDialog
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
            System.Windows.Forms.Label label2;
            System.Windows.Forms.Label label3;
            this.XSC = new System.Windows.Forms.NumericUpDown();
            this.YSC = new System.Windows.Forms.NumericUpDown();
            this.OKBT = new System.Windows.Forms.Button();
            this.CancelBT = new System.Windows.Forms.Button();
            label1 = new System.Windows.Forms.Label();
            label2 = new System.Windows.Forms.Label();
            label3 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.XSC)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.YSC)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(12, 9);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(222, 13);
            label1.TabIndex = 0;
            label1.Text = "Translate map geometry by the given amount.";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new System.Drawing.Point(43, 40);
            label2.Name = "label2";
            label2.Size = new System.Drawing.Size(14, 13);
            label2.TabIndex = 1;
            label2.Text = "X";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new System.Drawing.Point(43, 66);
            label3.Name = "label3";
            label3.Size = new System.Drawing.Size(14, 13);
            label3.TabIndex = 3;
            label3.Text = "Y";
            // 
            // XSC
            // 
            this.XSC.Location = new System.Drawing.Point(63, 38);
            this.XSC.Maximum = new decimal(new int[] {
            32767,
            0,
            0,
            0});
            this.XSC.Minimum = new decimal(new int[] {
            32768,
            0,
            0,
            -2147483648});
            this.XSC.Name = "XSC";
            this.XSC.Size = new System.Drawing.Size(120, 20);
            this.XSC.TabIndex = 2;
            this.XSC.ValueChanged += new System.EventHandler(this.XSC_ValueChanged);
            // 
            // YSC
            // 
            this.YSC.Location = new System.Drawing.Point(63, 64);
            this.YSC.Maximum = new decimal(new int[] {
            32767,
            0,
            0,
            0});
            this.YSC.Minimum = new decimal(new int[] {
            32768,
            0,
            0,
            -2147483648});
            this.YSC.Name = "YSC";
            this.YSC.Size = new System.Drawing.Size(120, 20);
            this.YSC.TabIndex = 4;
            this.YSC.ValueChanged += new System.EventHandler(this.YSC_ValueChanged);
            // 
            // OKBT
            // 
            this.OKBT.Location = new System.Drawing.Point(45, 109);
            this.OKBT.Name = "OKBT";
            this.OKBT.Size = new System.Drawing.Size(75, 23);
            this.OKBT.TabIndex = 5;
            this.OKBT.Text = "OK";
            this.OKBT.UseVisualStyleBackColor = true;
            this.OKBT.Click += new System.EventHandler(this.OKButton_Click);
            // 
            // CancelBT
            // 
            this.CancelBT.Location = new System.Drawing.Point(126, 109);
            this.CancelBT.Name = "CancelBT";
            this.CancelBT.Size = new System.Drawing.Size(75, 23);
            this.CancelBT.TabIndex = 6;
            this.CancelBT.Text = "Cancel";
            this.CancelBT.UseVisualStyleBackColor = true;
            this.CancelBT.Click += new System.EventHandler(this.CancelButton_Click);
            // 
            // ShiftDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(247, 153);
            this.ControlBox = false;
            this.Controls.Add(this.CancelBT);
            this.Controls.Add(this.OKBT);
            this.Controls.Add(this.YSC);
            this.Controls.Add(label3);
            this.Controls.Add(this.XSC);
            this.Controls.Add(label2);
            this.Controls.Add(label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ShiftDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Shift";
            ((System.ComponentModel.ISupportInitialize)(this.XSC)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.YSC)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.NumericUpDown XSC;
        private System.Windows.Forms.NumericUpDown YSC;
        private System.Windows.Forms.Button OKBT;
        private System.Windows.Forms.Button CancelBT;
    }
}