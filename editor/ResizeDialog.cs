using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace editor
{
    public partial class ResizeDialog : Form
    {
        public ResizeDialog(uint w, uint h)
        {
            InitializeComponent();
            DialogResult = DialogResult.Cancel;
            WidthSC.Value = w;
            HeightSC.Value = h;
        }

        public int W { get; set; }
        public int H { get; set; }

        private void WidthSC_ValueChanged(object sender, EventArgs e)
        {
            W = (int)WidthSC.Value;
        }

        private void HeightSC_ValueChanged(object sender, EventArgs e)
        {
            H = (int)HeightSC.Value;
        }

        private void OKButton_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
            Close();
        }

        private void CancelButton_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }
    }
}
