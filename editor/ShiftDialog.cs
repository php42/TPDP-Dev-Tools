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
    public partial class ShiftDialog : Form
    {
        public int X { get; set; }
        public int Y { get; set; }

        public ShiftDialog()
        {
            InitializeComponent();
            DialogResult = DialogResult.Cancel;
        }

        private void XSC_ValueChanged(object sender, EventArgs e)
        {
            X = (int)XSC.Value;
        }

        private void YSC_ValueChanged(object sender, EventArgs e)
        {
            Y = (int)YSC.Value;
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
