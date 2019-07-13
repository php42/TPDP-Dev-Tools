using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;

namespace editor
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            try
            {
                Application.Run(new EditorMainWindow());
            }
            catch(Exception e)
            {
                MessageBox.Show("Unhandled Exception: " + e.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
