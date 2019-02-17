using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.IO;

namespace patcher
{
    public partial class Form1 : Form
    {
        private Process proc_;
        public delegate void do_the_thing(string msg);

        public Form1()
        {
            InitializeComponent();

            // detect game folder location
            var appdata_path = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            var path = appdata_path + "/FocasLens/幻想人形演舞-ユメノカケラ-/install.ini";
            if (!File.Exists(path))
            {
                path = appdata_path + "/FocasLens/幻想人形演舞/gn_enbu.ini";
                if (!File.Exists(path))
                    return;
            }

            try
            {
                var enc = Encoding.GetEncoding(932);
                var file = File.OpenRead(path);
                if(!(file.Length > 0))
                {
                    file.Close();
                    return;
                }

                var buf = new byte[file.Length];
                file.Read(buf, 0, (int)file.Length);
                file.Close();
                var str = enc.GetString(buf);
                if (String.IsNullOrEmpty(str))
                    return;
                var pos = str.IndexOf("InstallPath=");
                if (pos < 0)
                    return;
                pos += "InstallPath=".Length;
                var endpos = str.IndexOf("\r\n", pos);
                if (endpos < 0)
                    endpos = str.Length;
                textBox1.Text = str.Substring(pos, endpos - pos);
            }
            catch
            {
                return;
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (folderBrowserDialog1.ShowDialog() == DialogResult.OK)
                textBox1.Text = folderBrowserDialog1.SelectedPath;
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() == DialogResult.OK)
                textBox2.Text = openFileDialog1.FileName;
        }

        public void append_msg(string msg)
        {
            textBox3.AppendText(msg);
        }

        public void stdout_handler(object proc, DataReceivedEventArgs data)
        {
            if(!String.IsNullOrEmpty(data.Data))
            {
                if(textBox3.InvokeRequired)
                    textBox3.Invoke(new do_the_thing(append_msg), new object[] { (data.Data + "\r\n") });
                else
                    textBox3.AppendText(data.Data + "\r\n");
            }
        }

        public void onexit(Object source, EventArgs e)
        {
            proc_.Close();
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if(!File.Exists("diffgen.exe"))
            {
                MessageBox.Show("Could not find diffgen.exe, please make sure it is in the same folder as this program", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if(!Directory.Exists(textBox1.Text))
            {
                MessageBox.Show("Invalid game folder path", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if(!File.Exists(textBox2.Text))
            {
                MessageBox.Show("Invalid patch file path", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            textBox3.Clear();

            String args = "-i \"" + textBox1.Text + "\" -o \"" + textBox2.Text + "\" -p";
            textBox3.AppendText("diffgen.exe " + args + "\r\n");
            
            proc_ = new Process();
            proc_.StartInfo.FileName = "diffgen.exe";
            proc_.StartInfo.Arguments = args;
            proc_.StartInfo.UseShellExecute = false;
            proc_.StartInfo.RedirectStandardOutput = true;
            proc_.StartInfo.RedirectStandardError = true;
            proc_.StartInfo.CreateNoWindow = true;
            proc_.EnableRaisingEvents = true;
            proc_.OutputDataReceived += new DataReceivedEventHandler(stdout_handler);
            proc_.ErrorDataReceived += new DataReceivedEventHandler(stdout_handler);
            proc_.Exited += new EventHandler(onexit);
            proc_.SynchronizingObject = this;

            try
            {
                proc_.Start();
                proc_.BeginOutputReadLine();
                proc_.BeginErrorReadLine();
            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
