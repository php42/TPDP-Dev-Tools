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

// Event tab of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private void LoadEvents()
        {
            EventMapCB.SelectedIndex = -1;
            EventMapCB.Items.Clear();

            foreach(var map in maps_)
            {
                EventMapCB.Items.Add(map.location_name);
            }
        }

        private void RefreshEvent()
        {
            var mapindex = EventMapCB.SelectedIndex;
            var id = (uint)EventIDSC.Value;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
                return;

            var obj = obs_data_.entries[id];

            EventObjIDSC.Value = obj.object_id;
            EventMovementSC.Value = obj.movement_mode;
            EventSpeedSC.Value = obj.movement_delay;
            EventArgSC.Value = obj.event_arg;
            EventIndexArgSC.Value = obj.event_index;

            var flagindex = (maps_[mapindex].id * 128) + (id / 8);
            var flag = (uint)event_flags_[flagindex];
            EventEnabledCB.CheckedChanged -= EventEnabledCB_CheckedChanged;
            EventEnabledCB.Checked = (flag & (1u << ((int)id % 8))) != 0;
            EventEnabledCB.CheckedChanged += EventEnabledCB_CheckedChanged;

            string str = obj.flags[0].ToString();
            for(var i = 1; i < obj.flags.Length; ++i)
                str += "," + obj.flags[i].ToString();
            EventFlagsTB.Text = str;
        }

        private void EventIDSC_ValueChanged(object sender, EventArgs e)
        {
            RefreshEvent();
        }

        private void EventObjIDSC_ValueChanged(object sender, EventArgs e)
        {
            var index = (uint)EventIDSC.Value;
            var id = (uint)EventObjIDSC.Value;
            if((fmf_data_ == null) || (obs_data_ == null))
                return;

            obs_data_.entries[index].object_id = id;
            EventObjectPB.Image = LoadObjectBmp(id);
        }

        private void EventTypeSC_ValueChanged(object sender, EventArgs e)
        {
            var index = (uint)EventIDSC.Value;
            var val = (uint)EventMovementSC.Value;
            if((fmf_data_ == null) || (obs_data_ == null))
                return;

            obs_data_.entries[index].movement_mode = val;
        }

        private void EventUnknownSC_ValueChanged(object sender, EventArgs e)
        {
            var index = (uint)EventIDSC.Value;
            var val = (uint)EventSpeedSC.Value;
            if((fmf_data_ == null) || (obs_data_ == null))
                return;

            obs_data_.entries[index].movement_delay = val;
        }

        private void EventArgSC_ValueChanged(object sender, EventArgs e)
        {
            var index = (uint)EventIDSC.Value;
            var val = (uint)EventArgSC.Value;
            if((fmf_data_ == null) || (obs_data_ == null))
                return;

            obs_data_.entries[index].event_arg = val;
        }

        private void EventIndexArgSC_ValueChanged(object sender, EventArgs e)
        {
            var index = (uint)EventIDSC.Value;
            var val = (uint)EventIndexArgSC.Value;
            if((fmf_data_ == null) || (obs_data_ == null))
                return;

            obs_data_.entries[index].event_index = val;
        }

        private void EventFlagsTB_TextChanged(object sender, EventArgs e)
        {
            var index = (uint)EventIDSC.Value;
            if((fmf_data_ == null) || (obs_data_ == null))
                return;

            try
            {
                var str = EventFlagsTB.Text;
                var flags = str.Split(',');
                for(var i = 0; i < Math.Min(flags.Length, 12); ++i)
                {
                    obs_data_.entries[index].flags[i] = uint.Parse(flags[i]);
                }
            }
            catch(Exception ex)
            {
                ErrMsg("Error parsing flag string: " + ex.Message);
            }
        }

        private void EventMapCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = EventMapCB.SelectedIndex;
            if(index > 0)
                SelectMap(index);
        }

        private void EventEnabledCB_CheckedChanged(object sender, EventArgs e)
        {
            var mapindex = EventMapCB.SelectedIndex;
            var id = (int)EventIDSC.Value;
            if((mapindex < 0) || (event_flags_ == null))
                return;

            var index = (maps_[mapindex].id * 128) + (id / 8);
            uint flag = (1u << (id % 8));
            if(EventEnabledCB.Checked)
                event_flags_[index] |= (byte)flag;
            else
                event_flags_[index] &= (byte)~flag;
        }

        private void EventReadmeBT_Click(object sender, EventArgs e)
        {
            var path = @".\docs\EVENT README.txt";
            if(!File.Exists(path))
                path = @"https://github.com/php42/TPDP-Dev-Tools/blob/master/docs/EVENT%20README.txt";
            Process.Start(path);
        }
    }
}
