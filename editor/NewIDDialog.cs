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
    public partial class NewIDDialog : Form
    {
        public int ID { get; set; }
        public NewIDDialog(string title, int max)
        {
            InitializeComponent();
            DialogResult = DialogResult.Cancel;
            Text = title;
            IDSC.Maximum = max;
            ID = 0;
        }

        private void IDSC_ValueChanged(object sender, EventArgs e)
        {
            ID = (int)IDSC.Value;
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
    }
}
