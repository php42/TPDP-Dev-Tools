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
using editor.json;
using System.Runtime.Serialization.Json;

// Skill tab of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private void LoadSkills(string filepath)
        {
            SkillDataCB.SelectedIndex = -1;
            SkillDataCB.Items.Clear();
            SkillDataCB.DisplayMember = "Item1";
            SkillDataCB.ValueMember = "Item2";
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(SkillJson));
            var buf = File.ReadAllBytes(filepath);
            MemoryStream s = new MemoryStream(buf);
            var tmp = (SkillJson)ser.ReadObject(s);
            foreach(var skill in tmp.skills)
            {
                if(skill_names_.ContainsKey(skill.id))
                {
                    skills_[skill.id] = skill;
                    SkillDataCB.Items.Add(new Tuple<string, uint>(skill_names_[skill.id], skill.id));
                }
            }
        }

        private void SkillDataCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = SkillDataCB.SelectedIndex;
            if(index < 0)
                return;

            var id = ((Tuple<string, uint>)SkillDataCB.Items[index]).Item2;
            var skill = skills_[id];

            SkillDataIDSC.Value = id;
            SkillDataTB.Text = skill.name;
            SkillDataElementCB.SelectedIndex = SkillDataElementCB.FindStringExact(skill.element);
            SkillDataTypeCB.SelectedIndex = SkillDataTypeCB.FindStringExact(skill.type);
            SkillDataSPSC.Value = skill.sp;
            SkillDataAccSC.Value = skill.accuracy;
            SkillDataPowerSC.Value = skill.power;
            SkillDataPrioSC.Value = skill.priority;
            SkillDataEffectChanceSC.Value = skill.effect_chance;
            SkillDataEffectIDSC.Value = skill.effect_id;
            SkillDataEffectTargetSC.Value = skill.effect_target;
            SkillDataEffectTypeSC.Value = skill.ynk_effect_type;
        }

        private void SkillDataChanged(object sender, EventArgs e)
        {
            var index = SkillDataCB.SelectedIndex;
            if(index < 0)
                return;

            var id = ((Tuple<string, uint>)SkillDataCB.Items[index]).Item2;
            var skill = skills_[id];

            // spaghetti
            if(sender == (object)SkillDataTB)                   // skill name
            {
                var name = SkillDataTB.Text;
                skills_[id].name = name;

                SkillDataCB.SelectedIndexChanged -= SkillDataChanged;
                SkillDataCB.Items[index] = new Tuple<string, uint>(name, id);
                SkillDataCB.SelectedIndexChanged += SkillDataChanged;
            }
            else if(sender == (object)SkillDataElementCB)       // skill element
            {
                if(SkillDataElementCB.SelectedIndex < 0)
                    return;
                var element = SkillDataElementCB.Text;
                skills_[id].element = element;
            }
            else if(sender == (object)SkillDataTypeCB)          // skill type
            {
                if(SkillDataTypeCB.SelectedIndex < 0)
                    return;
                var type = SkillDataTypeCB.Text;
                skills_[id].type = type;
            }
            else if(sender == (object)SkillDataSPSC)            // skill sp
            {
                var val = SkillDataSPSC.Value;
                skills_[id].sp = (uint)val;
            }
            else if(sender == (object)SkillDataAccSC)           // skill accuracy
            {
                var val = SkillDataAccSC.Value;
                skills_[id].accuracy = (uint)val;
            }
            else if(sender == (object)SkillDataPowerSC)         // skill power
            {
                var val = SkillDataPowerSC.Value;
                skills_[id].power = (uint)val;
            }
            else if(sender == (object)SkillDataPrioSC)          // skill priority
            {
                var val = SkillDataPrioSC.Value;
                skills_[id].priority = (int)val;
            }
            else if(sender == (object)SkillDataEffectChanceSC)  // effect chance
            {
                var val = SkillDataEffectChanceSC.Value;
                skills_[id].effect_chance = (uint)val;
            }
            else if(sender == (object)SkillDataEffectIDSC)      // effect id
            {
                var val = SkillDataEffectIDSC.Value;
                skills_[id].effect_id = (uint)val;
            }
            else if(sender == (object)SkillDataEffectTargetSC)  // effect target
            {
                var val = SkillDataEffectTargetSC.Value;
                skills_[id].effect_target = (uint)val;
            }
            else if(sender == (object)SkillDataEffectTypeSC)    // effect type
            {
                var val = SkillDataEffectTypeSC.Value;
                skills_[id].ynk_effect_type = (uint)val;
            }
        }
    }
}
