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

// Design tab of MainWindow

namespace editor
{
    public partial class EditorMainWindow : Form
    {
        private FmfJson fmf_data_;
        private ObsJson obs_data_;
        private ChipJson[] chp_data_ = new ChipJson[4];
        private Bitmap[] chp_bmps_ = new Bitmap[4];
        private byte[][] map_layers_ = new byte[13][];
        private Dictionary<uint, Bitmap> objects_ = new Dictionary<uint, Bitmap>();

        private MapDisplay map_display_;
        private TilesetDisplay tileset_display_;

        private void InitDesign()
        {
            for(var i = 0; i < 8; ++i)
            {
                LayerVisibiltyCB.SetItemChecked(i, true);
            }
            LayerVisibiltyCB.SetItemChecked(10, true);
            LayerVisibiltyCB.SetItemChecked(11, true);

            map_display_ = new MapDisplay();
            map_display_.Location = new Point(5, 5);
            map_display_.MouseDown += MapDisplay_MouseClick;
            map_display_.MouseMove += MapDisplay_MouseClick;
            map_display_.MouseMove += MapDisplay_MouseMove;
            MapImgPanel.Controls.Add(map_display_);

            tileset_display_ = new TilesetDisplay();
            tileset_display_.Location = new Point(5, 5);
            tileset_display_.MouseClick += TilesetDisplay_MouseClick;
            TilesetImgPanel.Controls.Add(tileset_display_);
        }

        private void LoadDesign()
        {
            MapDesignCB.SelectedIndex = -1;
            MapDesignCB.Items.Clear();

            foreach(var map in maps_)
            {
                MapDesignCB.Items.Add(map.location_name);
            }
        }

        private void LoadFmf(string path)
        {
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(FmfJson));
            var buf = File.ReadAllBytes(path);
            MemoryStream s = new MemoryStream(buf);
            var fmf = (FmfJson)ser.ReadObject(s);
            fmf.filepath = path;
            fmf_data_ = fmf;
        }

        private void LoadObs(string path)
        {
            DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(ObsJson));
            var buf = File.ReadAllBytes(path);
            MemoryStream s = new MemoryStream(buf);
            var obs = (ObsJson)ser.ReadObject(s);
            obs.filepath = path;
            obs_data_ = obs;
        }

        private ChipJson LoadChp(uint id)
        {
            try
            {
                var path = id.ToString();
                while(path.Length < 3)
                    path = "0" + path;
                path = working_dir_ + (is_ynk_ ? "/gn_dat5.arc/map/chip/" : "/gn_dat3.arc/map/chip/") + path + ".json";

                DataContractJsonSerializer ser = new DataContractJsonSerializer(typeof(ChipJson));
                var buf = File.ReadAllBytes(path);
                MemoryStream s = new MemoryStream(buf);
                var chp = (ChipJson)ser.ReadObject(s);
                return chp;
            }
            catch(Exception /*ex*/)
            {
                return new ChipJson();
            }
        }

        private Bitmap LoadChpBmp(uint id)
        {
            try
            {
                var path = id.ToString();
                while(path.Length < 3)
                    path = "0" + path;
                path = working_dir_ + (is_ynk_ ? "/gn_dat5.arc/map/chip/" : "/gn_dat3.arc/map/chip/") + path + ".png";

                var bmp = new Bitmap(path);
                return bmp;
            }
            catch(Exception /*ex*/)
            {
                var str = working_dir_ + (is_ynk_ ? "/gn_dat5.arc/map/chip/000.png" : "/gn_dat3.arc/map/chip/000.png");
                return new Bitmap(str);
            }
        }

        private Bitmap LoadObjectBmp(uint id)
        {
            try
            {
                var path = id.ToString();
                while(path.Length < 3)
                    path = "0" + path;
                path = working_dir_ + (is_ynk_ ? "/gn_dat5.arc/map/obj/" : "/gn_dat3.arc/map/obj/") + path + ".png";
                var path2 = path.Replace(".png", "_000.png");
                if(File.Exists(path2))
                    path = path2;
                else if(!File.Exists(path))
                    path = working_dir_ + (is_ynk_ ? "/gn_dat5.arc/map/obj/000.png" : "/gn_dat3.arc/map/obj/000.png");

                var bmp = new Bitmap(path);
                return bmp;
            }
            catch(Exception /*ex*/)
            {
                return new Bitmap(96, 128);
            }
        }

        private Bitmap RenderMap(List<int> layers, Rectangle clipping_rect)
        {
            if((fmf_data_ == null) || (obs_data_ == null) || (MapDesignCB.SelectedIndex < 0))
                return new Bitmap(32, 32);

            var bmp = new Bitmap(clipping_rect.Width, clipping_rect.Height);

            var width = (int)fmf_data_.width;
            var height = (int)fmf_data_.height;

            using(var g = Graphics.FromImage(bmp))
            {
                g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;
                g.Clear(Color.Transparent);
                g.CompositingMode = System.Drawing.Drawing2D.CompositingMode.SourceOver;

                for(var i = 0; i < 13; ++i)
                {
                    if(!layers.Contains(i))
                        continue;

                    if(i < 8)
                    {
                        for(var j = 0; j < (width * height); ++j)
                        {
                            var tileset = (uint)map_layers_[i][(j * 2) + 1];
                            var index = (int)chp_data_[tileset].index_map[(uint)map_layers_[i][j * 2]];
                            var src_rect = new Rectangle((index % 8) * 16, (index / 8) * 16, 16, 16);
                            var dst_rect = new Rectangle(((j % width) * 16) - clipping_rect.X, ((j / width) * 16) - clipping_rect.Y, 16, 16);

                            if((index != 0) || (tileset != 0))
                                g.DrawImage(chp_bmps_[tileset], dst_rect, src_rect, GraphicsUnit.Pixel);
                        }
                    }
                    else
                    {
                        using(var fnt = new Font("Arial", 9))
                        using(var bkgrnd_brush = new SolidBrush(Color.DarkSlateGray))
                        using(var text_brush = new SolidBrush(Color.White))
                        {
                            for(var j = 0; j < (width * height); ++j)
                            {
                                var index = (uint)BitConverter.ToUInt16(map_layers_[i], j * 2);
                                var src_rect = new Rectangle(0, 0, 32, 32);
                                var dst_rect = new Rectangle((((j % width) * 16) - 8) - clipping_rect.X, (((j / width) * 16) - 16) - clipping_rect.Y, 32, 32);
                                var obj_id = obs_data_.entries[index].object_id;

                                if(index > 0)
                                {
                                    if((i != 8) && (obj_id > 0))
                                    {
                                        if(!objects_.ContainsKey(obj_id))
                                            objects_[obj_id] = LoadObjectBmp(obj_id);
                                        var obj_bmp = objects_[obj_id];
                                        g.DrawImage(obj_bmp, dst_rect, src_rect, GraphicsUnit.Pixel);
                                    }

                                    if(DesignLabelCB.Checked)
                                    {
                                        var str = index.ToString();
                                        var fmt = new StringFormat();
                                        fmt.Alignment = StringAlignment.Center;
                                        fmt.LineAlignment = StringAlignment.Center;
                                        fmt.FormatFlags = StringFormatFlags.NoWrap;
                                        fmt.Trimming = StringTrimming.None;
                                        var text_sz = g.MeasureString(str, fnt);
                                        var text_rect = dst_rect;
                                        text_rect.Width = (int)text_sz.Width;
                                        text_rect.Height = (int)text_sz.Height;
                                        text_rect.X += ((32 - text_rect.Width) / 2);
                                        text_rect.Y += 16 + ((16 - text_rect.Height) / 2);
                                        g.FillRectangle(bkgrnd_brush, text_rect);
                                        g.DrawString(str, fnt, text_brush, text_rect, fmt);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return bmp;
        }

        private void RenderMap(List<int> layers)
        {
            if((fmf_data_ == null) || (obs_data_ == null) || (MapDesignCB.SelectedIndex < 0))
                return;
            var rect = new Rectangle(0, 0, (int)fmf_data_.width * 16, (int)fmf_data_.height * 16);
            map_display_.SetBitmap(RenderMap(layers, rect));
        }

        private void RenderMap()
        {
            List<int> layers = new List<int>();
            foreach(var i in LayerVisibiltyCB.CheckedIndices)
                layers.Add((int)i);
            RenderMap(layers);
        }

        private void UpdateMapIndex(uint layer, uint index, uint value)
        {
            if((fmf_data_ == null) || (obs_data_ == null) || (MapDesignCB.SelectedIndex < 0))
                return;

            var buf = BitConverter.GetBytes((ushort)value);
            map_layers_[layer][index * 2] = buf[0];
            map_layers_[layer][(index * 2) + 1] = buf[1];

            List<int> layers = new List<int>();
            foreach(var i in LayerVisibiltyCB.CheckedIndices)
                layers.Add((int)i);

            var x = ((int)(index % fmf_data_.width) * 16) - 32;
            var y = ((int)(index / fmf_data_.width) * 16) - 16;

            var rect = new Rectangle(x, y, 64, 32);

            map_display_.UpdateIndex(RenderMap(layers, rect), index);
        }

        private void MapDesignCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var index = MapDesignCB.SelectedIndex;
            if(index < 0)
                return;

            var map = maps_[index];
            var fmfpath = map.filepath.Replace(".json", "_fmf.json");
            var obspath = map.filepath.Replace(".json", "_obs.json");

            try
            {
                LoadFmf(fmfpath);
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to load file: " + fmfpath + "\r\n" + ex.Message);
                return;
            }

            try
            {
                LoadObs(obspath);
            }
            catch(Exception ex)
            {
                ErrMsg("Failed to load file: " + obspath + "\r\n" + ex.Message);
                return;
            }

            for(uint i = 0; i < 4; ++i)
            {
                chp_data_[i] = LoadChp(map.tilesets[i]);
                chp_bmps_[i] = LoadChpBmp(map.tilesets[i]);
            }
            tileset_display_.SetBitmap(chp_bmps_[(int)BrushTilesetSC.Value]);
            var brush_val = (uint)BrushValueSC.Value;
            var tileset_index = (uint)BrushTilesetSC.Value;
            if(brush_val < 256)
                tileset_display_.SelectedIndex = (int)chp_data_[tileset_index].index_map[brush_val];

            Tileset1SC.ValueChanged -= TilesetSC_ValueChanged;
            Tileset2SC.ValueChanged -= TilesetSC_ValueChanged;
            Tileset3SC.ValueChanged -= TilesetSC_ValueChanged;
            Tileset4SC.ValueChanged -= TilesetSC_ValueChanged;
            Tileset1SC.Value = map.tilesets[0];
            Tileset2SC.Value = map.tilesets[1];
            Tileset3SC.Value = map.tilesets[2];
            Tileset4SC.Value = map.tilesets[3];
            Tileset1SC.ValueChanged += TilesetSC_ValueChanged;
            Tileset2SC.ValueChanged += TilesetSC_ValueChanged;
            Tileset3SC.ValueChanged += TilesetSC_ValueChanged;
            Tileset4SC.ValueChanged += TilesetSC_ValueChanged;

            for(var i = 0; i < 13; ++i)
            {
                map_layers_[i] = Convert.FromBase64String(fmf_data_.layers[i]);
            }

            RenderMap();
        }

        private void BrushTilesetSC_ValueChanged(object sender, EventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
                return;

            var index = (uint)BrushValueSC.Value;
            var tileset_index = (uint)BrushTilesetSC.Value;
            tileset_display_.SetBitmap(chp_bmps_[tileset_index]);
            if(index < 256)
                tileset_display_.SelectedIndex = (int)chp_data_[tileset_index].index_map[index];
        }

        private void LayerVisibiltyCB_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            List<int> layers = new List<int>();
            foreach(var i in LayerVisibiltyCB.CheckedIndices)
            {
                if(((int)i == e.Index) && (e.NewValue != CheckState.Checked))
                    continue;
                layers.Add((int)i);
            }
            if(e.NewValue == CheckState.Checked)
                layers.Add(e.Index);
            RenderMap(layers);
        }

        private void TilesetSC_ValueChanged(object sender, EventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
                return;
            int index;
            if(sender == (object)Tileset1SC)
                index = 0;
            else if(sender == (object)Tileset2SC)
                index = 1;
            else if(sender == (object)Tileset3SC)
                index = 2;
            else if(sender == (object)Tileset4SC)
                index = 3;
            else
                return;

            var val = (uint)((NumericUpDown)sender).Value;
            maps_[mapindex].tilesets[index] = val;
            chp_data_[index] = LoadChp(val);
            chp_bmps_[index] = LoadChpBmp(val);

            if(index == BrushTilesetSC.Value) // refresh tileset img
            {
                tileset_display_.SetBitmap(chp_bmps_[index]);
                var brush_val = (uint)BrushValueSC.Value;
                if(brush_val < 256)
                    tileset_display_.SelectedIndex = (int)chp_data_[index].index_map[brush_val];
            }

            RenderMap();
        }

        private void BrushLayerCB_SelectedIndexChanged(object sender, EventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
                return;
            var index = BrushLayerCB.SelectedIndex;

            if(index < 8)
                BrushValueSC.Maximum = 255;
            else
                BrushValueSC.Maximum = 65535;
        }

        private void BrushValueSC_ValueChanged(object sender, EventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
                return;

            var index = (uint)BrushValueSC.Value;
            if(index < 256)
            {
                var tileset_index = (uint)BrushTilesetSC.Value;
                tileset_display_.SelectedIndex = (int)chp_data_[tileset_index].index_map[index];
            }
        }

        private void TilesetDisplay_MouseClick(Object sender, MouseEventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
                return;

            var tileset_index = (uint)BrushTilesetSC.Value;
            var index = (e.X / 16) + ((e.Y / 16) * 8);
            uint val = 0;
            for(uint i = 0; i < 256; ++i) // find the first frame of the selected animation
            {
                var v = chp_data_[tileset_index].index_map[i];
                if((v <= index) && (v > chp_data_[tileset_index].index_map[val]))
                    val = i;
                if(val == index)
                    break;
            }

            BrushValueSC.Value = val;
        }


        private void MapDisplay_MouseClick(Object sender, MouseEventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            var layer = BrushLayerCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0) || (layer < 0))
                return;

            if((e.Button != MouseButtons.Left) && (e.Button != MouseButtons.Right))
                return;

            if((e.X < 0) || (e.Y < 0))
                return;

            if(((e.X / 16) >= fmf_data_.width) || ((e.Y / 16) >= fmf_data_.height))
                return;

            var tileset_index = (uint)BrushTilesetSC.Value;
            var brush_val = (uint)BrushValueSC.Value;
            var index = (e.X / 16) + ((e.Y / 16) * fmf_data_.width);

            if(e.Button == MouseButtons.Right)
            {
                tileset_index = 0;
                brush_val = 0;
            }

            if(layer < 8)
            {
                if(brush_val >= 256)
                    return;
                brush_val = (tileset_index * 256) + brush_val;
            }

            if(BitConverter.ToUInt16(map_layers_[layer], (int)index * 2) == brush_val)
                return;

            if(ModifierKeys == Keys.Control)
            {
                if(layer < 8)
                {
                    BrushTilesetSC.Value = map_layers_[layer][(index * 2) + 1];
                    BrushValueSC.Value = map_layers_[layer][index * 2];
                }
                else
                {
                    BrushValueSC.Value = BitConverter.ToUInt16(map_layers_[layer], (int)index * 2);
                }

                return;
            }

            UpdateMapIndex((uint)layer, (uint)index, brush_val);
        }

        private void DesignSaveBT_Click(object sender, EventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
                return;

            for(var i = 0; i < 13; ++i)
                fmf_data_.layers[i] = Convert.ToBase64String(map_layers_[i]);

            try
            {
                var ser = new DataContractJsonSerializer(typeof(FmfJson));
                var s = new MemoryStream();
                ser.WriteObject(s, fmf_data_);
                File.WriteAllBytes(fmf_data_.filepath, s.ToArray());
            }
            catch(Exception ex)
            {
                ErrMsg("Error saving file: " + fmf_data_.filepath + "\r\n" + ex.Message);
                return;
            }

            var map = maps_[mapindex];
            try
            {
                var ser = new DataContractJsonSerializer(typeof(MadJson));
                var s = new MemoryStream();
                ser.WriteObject(s, map);
                File.WriteAllBytes(map.filepath, s.ToArray());
            }
            catch(Exception ex)
            {
                ErrMsg("Error saving map: " + map.filepath + "\r\n" + ex.Message);
            }

            MessageBox.Show("Map save successful.", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private void DesignLabelCB_CheckedChanged(object sender, EventArgs e)
        {
            RenderMap();
        }

        private void DesignResizeBT_Click(object sender, EventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
            {
                ErrMsg("No map loaded.");
                return;
            }

            using(var dialog = new ResizeDialog(fmf_data_.width, fmf_data_.height))
            {
                if(dialog.ShowDialog() == DialogResult.OK)
                {
                    var w = dialog.W;
                    var h = dialog.H;

                    var new_sz = w * h * 2;

                    var old_stride = (int)(fmf_data_.width * 2);
                    var new_stride = w * 2;

                    var copy_sz = Math.Min(old_stride, new_stride);

                    for(var i = 0; i < fmf_data_.num_layers; ++i)
                    {
                        var buf = new byte[new_sz];
                        for(var j = 0; j < Math.Min(h, fmf_data_.height); ++j)
                        {
                            var src_pos = j * old_stride;
                            var dst_pos = j * new_stride;
                            Array.Copy(map_layers_[i], src_pos, buf, dst_pos, copy_sz);
                        }
                        map_layers_[i] = buf;
                    }

                    fmf_data_.width = (uint)w;
                    fmf_data_.height = (uint)h;
                    fmf_data_.payload_length = (uint)new_sz * fmf_data_.num_layers;
                    RenderMap();
                }
            }
        }

        private void DesignShiftBT_Click(object sender, EventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
            {
                ErrMsg("No map loaded.");
                return;
            }

            using(var dialog = new ShiftDialog())
            {
                if(dialog.ShowDialog() == DialogResult.OK)
                {
                    var x = dialog.X;
                    var y = dialog.Y;

                    var w = (int)fmf_data_.width;
                    var h = (int)fmf_data_.height;

                    if((Math.Abs(x) >= w) || (Math.Abs(y) >= h))
                    {
                        ErrMsg("Coordinates out of range!");
                        return;
                    }

                    var layer_sz = w * h * 2;
                    var stride = w * 2;

                    var copy_sz = stride - (Math.Abs(x) * 2);
                    var src_offset = Math.Max(0, -x) * 2;
                    var dst_offset = Math.Max(0, x) * 2;

                    for(var i = 0; i < fmf_data_.num_layers; ++i)
                    {
                        var buf = new byte[layer_sz];
                        for(var j = 0; j < h; ++j)
                        {
                            var k = (j - y);
                            if((k < 0) || (k >= h))
                                continue;

                            var src_pos = (k * stride) + src_offset;
                            var dst_pos = (j * stride) + dst_offset;
                            Array.Copy(map_layers_[i], src_pos, buf, dst_pos, copy_sz);
                        }
                        map_layers_[i] = buf;
                    }

                    RenderMap();
                }
            }
        }

        private void DesignClearBT_Click(object sender, EventArgs e)
        {
            var mapindex = MapDesignCB.SelectedIndex;
            if((fmf_data_ == null) || (obs_data_ == null) || (mapindex < 0))
            {
                ErrMsg("No map loaded.");
                return;
            }

            if(MessageBox.Show("Are you sure you want to clear the entire map?", "Confirm", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
            {
                var layer_sz = fmf_data_.width * fmf_data_.height * 2;
                for(var i = 0; i < map_layers_.Length; ++i)
                {
                    for(var j = 0; j < layer_sz; ++j)
                        map_layers_[i][j] = 0;
                }
                RenderMap();
            }
        }

        private void MapDisplay_MouseMove(object sender, MouseEventArgs e)
        {
            var str = (e.X / 16).ToString() + ", " + (e.Y / 16).ToString();
            if(DesignCoordLabel.Text != str)
                DesignCoordLabel.Text = str;
        }
    }
}
