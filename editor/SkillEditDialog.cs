using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace editor
{
    public partial class SkillEditDialog : Form
    {
        private List<ComboBox> style_cbs_ = new List<ComboBox>();
        private List<ComboBox> base_cbs_ = new List<ComboBox>();
        private List<Tuple<uint, string>> skillnames_ = new List<Tuple<uint, string>>();

        public List<uint> StyleSkills { get; set; }
        public List<uint> BaseSkills { get; set; }

        public SkillEditDialog(Dictionary<uint, string> skillnames, IList<uint> style_skills, IList<uint> base_skills)
        {
            InitializeComponent();

            skillnames_.Add(new Tuple<uint, string>(0, "None"));
            foreach(var it in skillnames)
            {
                if(it.Key != 0)
                    skillnames_.Add(new Tuple<uint, string>(it.Key, it.Value));
            }

            StyleSkills = null;

            style_cbs_.Add(CBSS1);
            style_cbs_.Add(CBSS2);
            style_cbs_.Add(CBSS3);
            style_cbs_.Add(CBSS4);
            style_cbs_.Add(CBLU1);
            style_cbs_.Add(CBLU2);
            style_cbs_.Add(CBLU3);
            style_cbs_.Add(CBLU4);
            style_cbs_.Add(CBLU5);
            style_cbs_.Add(CBLU6);
            style_cbs_.Add(CBLU7);
            style_cbs_.Add(CB701);
            style_cbs_.Add(CB702);
            style_cbs_.Add(CB703);
            style_cbs_.Add(CB704);
            style_cbs_.Add(CB705);
            style_cbs_.Add(CB706);
            style_cbs_.Add(CB707);
            style_cbs_.Add(CB708);
            style_cbs_.Add(CBLVL100);

            base_cbs_.Add(CBBase1);
            base_cbs_.Add(CBBase2);
            base_cbs_.Add(CBBase3);
            base_cbs_.Add(CBBase4);
            base_cbs_.Add(CBBase5);

            var i = 0;
            foreach(var it in style_cbs_)
            {
                it.DataSource = new BindingSource(skillnames_, null);
                it.SelectedIndex = -1;
                it.DisplayMember = "Item2";
                it.ValueMember = "Item1";

                var skillid = style_skills[i++];

                it.SelectedIndex = skillnames_.FindIndex(j => j.Item1 == skillid);
            }

            i = 0;
            foreach(var it in base_cbs_)
            {
                it.DataSource = new BindingSource(skillnames_, null);
                it.SelectedIndex = -1;
                it.DisplayMember = "Item2";
                it.ValueMember = "Item1";

                var skillid = base_skills[i++];

                it.SelectedIndex = skillnames_.FindIndex(j => j.Item1 == skillid);
            }
        }

        private void SkillEditDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            StyleSkills = new List<uint>();
            BaseSkills = new List<uint>();

            foreach(var it in style_cbs_)
            {
                if(it.SelectedIndex >= 0)
                    StyleSkills.Add((uint)it.SelectedValue);
                else
                    StyleSkills.Add(0);
            }

            foreach(var it in base_cbs_)
            {
                if(it.SelectedIndex >= 0)
                    BaseSkills.Add((uint)it.SelectedValue);
                else
                    BaseSkills.Add(0);
            }
        }
    }
}
