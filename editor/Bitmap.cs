using System.Drawing;
using System.Drawing.Drawing2D;

namespace editor.bitmap
{
    class BitmapUtil
    {
        public static Bitmap Resize(Bitmap bmp, int w, int h)
        {
            var ret = new Bitmap(w, h);
            using(var g = Graphics.FromImage(ret))
            {
                var src = new Rectangle(0, 0, bmp.Width, bmp.Height);
                var dst = new Rectangle(0, 0, w, h);
                g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                g.CompositingMode = CompositingMode.SourceCopy;
                g.CompositingQuality = CompositingQuality.HighQuality;
                g.PixelOffsetMode = PixelOffsetMode.HighQuality;
                g.DrawImage(bmp, dst, src, GraphicsUnit.Pixel);
            }

            return ret;
        }
    }
}