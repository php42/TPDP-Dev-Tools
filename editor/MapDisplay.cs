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
using editor.bitmap;
using System.Runtime.Serialization.Json;
using System.Text.RegularExpressions;

namespace editor
{
    class MapDisplay : Control
    {
        private Bitmap bmp_;

        public int Zoom { get; set; }

        public MapDisplay()
        {
            this.Cursor = Cursors.Hand;
            this.DoubleBuffered = true;
            Zoom = 1;
        }

        public void SetBitmap(Bitmap bmp)
        {
            if(Zoom == 1)
            {
                bmp_ = bmp;
            }
            else
            {
                var w = bmp.Width * Zoom;
                var h = bmp.Height * Zoom;
                bmp_ = BitmapUtil.Resize(bmp, w, h);
            }

            SetClientSizeCore(bmp_.Width, bmp_.Height);
            Invalidate();
            Update();
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            e.Graphics.CompositingMode = CompositingMode.SourceCopy;
            e.Graphics.CompositingQuality = CompositingQuality.HighQuality;
            e.Graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;

            // Fix alignment for autoscroll repaints
            // Cause winforms is the actual worst.
            var tile_sz = 16 * Zoom;
            var rect = e.ClipRectangle;
            var dx = rect.X % tile_sz;
            var dy = rect.Y % tile_sz;
            rect.X -= dx;
            rect.Y -= dy;
            rect.Width += dx;
            rect.Height += dy;
            if((rect.Width % tile_sz) != 0)
                rect.Width += tile_sz - (rect.Width % tile_sz);
            if((rect.Height % tile_sz) != 0)
                rect.Height += tile_sz - (rect.Height % tile_sz);
            e.Graphics.DrawImage(bmp_, rect, rect, GraphicsUnit.Pixel);
        }

        public void UpdateRegion(Bitmap bmp, int x, int y, bool repaint)
        {
            using(var g = Graphics.FromImage(bmp_))
            {
                g.InterpolationMode = InterpolationMode.NearestNeighbor;
                g.CompositingMode = CompositingMode.SourceCopy;
                g.CompositingQuality = CompositingQuality.HighQuality;
                g.PixelOffsetMode = PixelOffsetMode.HighQuality;

                if(Zoom != 1)
                {
                    var src = new Rectangle(0, 0, bmp.Width, bmp.Height);
                    var dst = new Rectangle(x * Zoom, y * Zoom, bmp.Width * Zoom, bmp.Height * Zoom);
                    g.DrawImage(bmp, dst, src, GraphicsUnit.Pixel);
                    Invalidate(dst);
                }
                else
                {
                    g.DrawImage(bmp, x, y);
                    Invalidate(new Rectangle(x, y, bmp.Width, bmp.Height));
                }
            }

            if(repaint)
                Update();
        }
    }
}