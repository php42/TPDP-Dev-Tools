using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace editor
{
    public partial class PatchDialog : Form
    {
        public string PatchPath { get; set; }
        public string DirPath { get; set; }

        public PatchDialog(string path)
        {
            InitializeComponent();
            DirTB.Text = path;
            DialogResult = DialogResult.Cancel;
        }

        private void BrowseBT_Click(object sender, EventArgs e)
        {
            if(BrowseDialog.ShowDialog() == DialogResult.OK)
            {
                DirTB.Text = BrowseDialog.SelectedPath;
            }
        }

        private void GenerateBT_Click(object sender, EventArgs e)
        {
            if(!Directory.Exists(DirTB.Text))
            {
                MessageBox.Show(this, "Invalid game folder.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if(SaveDialog.ShowDialog() == DialogResult.OK)
            {
                PatchPath = SaveDialog.FileName;
                DirPath = DirTB.Text;
                DialogResult = DialogResult.OK;
                Close();
            }
        }
    }
}
