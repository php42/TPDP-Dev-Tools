using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private string last_evs_;

        private void OpenEvsFile(string filepath)
        {
            var fileInfo = new FileInfo(filepath);
            if(!fileInfo.Exists)
            {
                ErrMsg("File does not exist.");
            }
            else if(fileInfo.Length != 640)
            {
                ErrMsg("Can't open selected file as EVS file.");
            }

            last_evs_ = filepath;
            try
            {
                var buf = File.ReadAllBytes(filepath);
                if(buf.Length != 640)
                    throw new Exception("Incorrect file size.");

                string outfmt = EvsHexCB.Checked ? "X" : "G";

                EvsTB.Clear();
                var lines = new string[32];
                var fields = new string[10];
                for(int i = 0; i < 32; ++i)
                {
                    for(int j = 0; j < 10; ++j)
                    {
                        int index = (i * 20) + (j * 2);
                        var val = BitConverter.ToUInt16(buf, index);
                        fields[j] = val.ToString(outfmt);
                    }
                    lines[i] = string.Join(", ", fields);
                }
                EvsTB.Lines = lines;
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to read EVS file: " + ex.Message);
            }
        }

        private void SaveEvsFile(string filepath)
        {
            try
            {
                var buf = new byte[640];
                var lines = EvsTB.Lines;
                int pos = 0;
                last_evs_ = filepath;

                int inbase = EvsHexCB.Checked ? 16 : 10;

                foreach(string line in lines)
                {
                    if(string.IsNullOrWhiteSpace(line))
                        continue;
                    if(pos >= 32)
                        throw new Exception("Too many lines.");

                    string[] fields = line.Split(',');
                    if(fields.Length != 10)
                        throw new Exception("line must have 10 fields.");

                    for(int i = 0; i < 10; ++i)
                    {
                        int index = (pos * 20) + (i * 2);
                        var val = BitConverter.GetBytes(Convert.ToUInt16(fields[i].Trim(), inbase));
                        buf[index] = val[0];
                        buf[index + 1] = val[1];
                    }
                    ++pos;
                }
                File.WriteAllBytes(filepath, buf);
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to write EVS file: " + ex.Message);
            }
        }

        private void EvsTB_DragEnter(object sender, DragEventArgs e)
        {
            e.Effect = DragDropEffects.Copy;
        }

        private void EvsTB_DragDrop(object sender, DragEventArgs e)
        {
            if(e.Data.GetDataPresent(DataFormats.FileDrop))
                OpenEvsFile(((string[])e.Data.GetData(DataFormats.FileDrop))[0]);
        }

        private void EvsOpenBT_Click(object sender, EventArgs e)
        {
            string path;
            using(var dlg = new OpenFileDialog())
            {
                if(!string.IsNullOrEmpty(working_dir_))
                    dlg.InitialDirectory = working_dir_ + "\\gn_dat5.arc\\script\\event";
                dlg.DefaultExt = "evs";
                dlg.Filter = "evs files (*.evs)|*.evs";
                dlg.Multiselect = false;

                if(dlg.ShowDialog() != DialogResult.OK)
                    return;
                path = dlg.FileName;
            }
            OpenEvsFile(path);
        }

        private void EvsSaveBT_Click(object sender, EventArgs e)
        {
            string path;
            using(var dlg = new SaveFileDialog())
            {
                if(!string.IsNullOrEmpty(last_evs_))
                {
                    dlg.InitialDirectory = Path.GetDirectoryName(last_evs_);
                    dlg.FileName = Path.GetFileName(last_evs_);
                }
                else if(!string.IsNullOrEmpty(working_dir_))
                    dlg.InitialDirectory = working_dir_ + "\\gn_dat5.arc\\script\\event";
                dlg.DefaultExt = "evs";
                dlg.Filter = "evs files (*.evs)|*.evs";
                if(dlg.ShowDialog() != DialogResult.OK)
                    return;
                path = dlg.FileName;
            }
            SaveEvsFile(path);
        }

        private void EvsNewBT_Click(object sender, EventArgs e)
        {
            var lines = new string[32];
            for(int i = 0; i < 32; ++i)
                lines[i] = "0, 0, 0, 0, 0, 0, 0, 0, 0, 0";
            EvsTB.Lines = lines;
        }

        private void EvsReadmeBT_Click(object sender, EventArgs e)
        {
            var path = @".\docs\EVS SCRIPT README.txt";
            if(!File.Exists(path))
                path = @"https://github.com/php42/TPDP-Dev-Tools/blob/master/docs/EVS%20SCRIPT%20README.txt";
            Process.Start(path);
        }

        private void EvsHexCB_CheckedChanged(object sender, EventArgs e)
        {
            if(EvsTB.TextLength == 0)
                return;

            try
            {
                int inbase = 10;
                string outfmt = "X";
                if(!EvsHexCB.Checked)
                {
                    inbase = 16;
                    outfmt = "G";
                }

                var lines = EvsTB.Lines;
                for(var i = 0; i < lines.Length; ++i)
                {
                    if(string.IsNullOrWhiteSpace(lines[i]))
                        continue;
                    var fields = lines[i].Split(',');
                    for(var j = 0; j < fields.Length; ++j)
                    {
                        fields[j] = Convert.ToUInt16(fields[j].Trim(), inbase).ToString(outfmt);
                    }
                    lines[i] = string.Join(", ", fields);
                }
                EvsTB.Lines = lines;
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to convert to/from hex: " + ex.Message);
                EvsHexCB.CheckedChanged -= EvsHexCB_CheckedChanged;
                EvsHexCB.Checked = !EvsHexCB.Checked;
                EvsHexCB.CheckedChanged += EvsHexCB_CheckedChanged;
            }
        }
    }
}
