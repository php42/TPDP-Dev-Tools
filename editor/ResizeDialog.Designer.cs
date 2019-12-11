namespace editor
{
    partial class ResizeDialog
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
            this.WidthSC = new System.Windows.Forms.NumericUpDown();
            this.HeightSC = new System.Windows.Forms.NumericUpDown();
            this.OKBT = new System.Windows.Forms.Button();
            this.CancelBT = new System.Windows.Forms.Button();
            label1 = new System.Windows.Forms.Label();
            label2 = new System.Windows.Forms.Label();
            ((System.ComponentModel.ISupportInitialize)(this.WidthSC)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.HeightSC)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(12, 31);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(35, 13);
            label1.TabIndex = 0;
            label1.Text = "Width";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new System.Drawing.Point(12, 57);
            label2.Name = "label2";
            label2.Size = new System.Drawing.Size(38, 13);
            label2.TabIndex = 1;
            label2.Text = "Height";
            // 
            // WidthSC
            // 
            this.WidthSC.Location = new System.Drawing.Point(56, 29);
            this.WidthSC.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.WidthSC.Name = "WidthSC";
            this.WidthSC.Size = new System.Drawing.Size(120, 20);
            this.WidthSC.TabIndex = 2;
            this.WidthSC.ValueChanged += new System.EventHandler(this.WidthSC_ValueChanged);
            // 
            // HeightSC
            // 
            this.HeightSC.Location = new System.Drawing.Point(56, 55);
            this.HeightSC.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.HeightSC.Name = "HeightSC";
            this.HeightSC.Size = new System.Drawing.Size(120, 20);
            this.HeightSC.TabIndex = 3;
            this.HeightSC.ValueChanged += new System.EventHandler(this.HeightSC_ValueChanged);
            // 
            // OKBT
            // 
            this.OKBT.Location = new System.Drawing.Point(26, 99);
            this.OKBT.Name = "OKBT";
            this.OKBT.Size = new System.Drawing.Size(75, 23);
            this.OKBT.TabIndex = 4;
            this.OKBT.Text = "OK";
            this.OKBT.UseVisualStyleBackColor = true;
            this.OKBT.Click += new System.EventHandler(this.OKButton_Click);
            // 
            // CancelBT
            // 
            this.CancelBT.Location = new System.Drawing.Point(107, 99);
            this.CancelBT.Name = "CancelBT";
            this.CancelBT.Size = new System.Drawing.Size(75, 23);
            this.CancelBT.TabIndex = 5;
            this.CancelBT.Text = "Cancel";
            this.CancelBT.UseVisualStyleBackColor = true;
            this.CancelBT.Click += new System.EventHandler(this.CancelButton_Click);
            // 
            // ResizeDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(209, 142);
            this.ControlBox = false;
            this.Controls.Add(this.CancelBT);
            this.Controls.Add(this.OKBT);
            this.Controls.Add(this.HeightSC);
            this.Controls.Add(this.WidthSC);
            this.Controls.Add(label2);
            this.Controls.Add(label1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ResizeDialog";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Resize";
            ((System.ComponentModel.ISupportInitialize)(this.WidthSC)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.HeightSC)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.NumericUpDown WidthSC;
        private System.Windows.Forms.NumericUpDown HeightSC;
        private System.Windows.Forms.Button OKBT;
        private System.Windows.Forms.Button CancelBT;
    }
}