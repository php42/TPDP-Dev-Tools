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
    public partial class NewMapDialog : Form
    {
        public string MapName { get; set; }
        public int MapID { get; set; }
        public int MapW { get; set; }
        public int MapH { get; set; }

        public NewMapDialog(int max)
        {
            InitializeComponent();
            MapName = "";
            MapW = 16;
            MapH = 16;
            DialogResult = DialogResult.Cancel;
            if(max > 0)
                IDSC.Maximum = max;
        }

        private void OKBT_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
            Close();
        }

        private void CancelBT_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }

        private void NameTB_TextChanged(object sender, EventArgs e)
        {
            MapName = NameTB.Text;
        }

        private void IDSC_ValueChanged(object sender, EventArgs e)
        {
            MapID = (int)IDSC.Value;
        }

        private void WSC_ValueChanged(object sender, EventArgs e)
        {
            MapW = (int)WSC.Value;
        }

        private void HSC_ValueChanged(object sender, EventArgs e)
        {
            MapH = (int)HSC.Value;
        }
    }
}
