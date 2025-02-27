﻿using System;
using System.Runtime.InteropServices;

namespace H264Sharp
{
    /// <summary>
    /// Image format converter
    /// </summary>
    public class Converter
    {
        /// <summary>
        /// Sets global configuration for converter.
        /// </summary>
        /// <param name="config"></param>
        public static void SetConfig(ConverterConfig config) => Defines.Native.ConverterSetConfig(config);

        /// <summary>
        /// Gets default configuration.
        /// </summary>
        /// <returns></returns>
        public static ConverterConfig GetDefaultConfig() => ConverterConfig.Default;

        /// <summary>
        /// Converts RGB,BGR,RGBA,BGRA to YUVI420P
        /// </summary>
        /// <param name="from"></param>
        /// <param name="yuv"></param>
        public static void Rgb2Yuv(ImageData from, YuvImage yuv)
        {
            unsafe
            {
                if (from.isManaged)
                {
                    fixed (byte* dp = &from.data[from.dataOffset])
                    {

                        var ugi = new UnsafeGenericImage()
                        {
                            ImageBytes = dp,
                            Width = from.Width,
                            Height = from.Height,
                            Stride = from.Stride,
                            ImgType = from.ImgType,
                        };
                        var refe = yuv.ToYUVImagePointer();
                        Defines.Native.RGBXtoYUV(ref ugi, ref refe);
                    }
                }
                else
                {
                    var ugi = new UnsafeGenericImage()
                    {
                        ImageBytes = (byte*)from.imageData.ToPointer(),
                        Width = from.Width,
                        Height = from.Height,
                        Stride = from.Stride,
                        ImgType = from.ImgType,
                    };
                    var refe = yuv.ToYUVImagePointer();
                    Defines.Native.RGBXtoYUV(ref ugi, ref refe);
                }
                
                   
            }
           
        }
        private Converter() { }

        /// <summary>
        /// Converts Rgb to Yuv420p
        /// </summary>
        /// <param name="from"></param>
        /// <param name="yuv"></param>
        public static void Rgb2Yuv(RgbImage from, YuvImage yuv)
        {
            unsafe
            {
                var ugi = new UnsafeGenericImage()
                {
                    ImageBytes = (byte*)from.ImageBytes.ToPointer(),
                    Width = from.Width,
                    Height = from.Height,
                    Stride = from.Stride,
                    ImgType = ImageType.Bgr,
                };
                var refe = yuv.ToYUVImagePointer();
                Defines.Native.RGBXtoYUV(ref ugi, ref refe);
            }
            
        }

        /// <summary>
        /// Converts YUV420p image to RGB format
        /// </summary>
        /// <param name="yuv"></param>
        /// <param name="image"></param>
        public static void Yuv2Rgb(YuvImage yuv,RgbImage image)
        {
            Yuv2Rgb(yuv.ToYUVImagePointer(), image);
        }

        /// <summary>
        /// Converts YUV420p image to RGB format
        /// </summary>
        /// <param name="yuv"></param>
        /// <param name="image"></param>
        public static void Yuv2Rgb(YUVImagePointer yuv, RgbImage image)
        {
            var rgb = new RGBImagePointer(image.Width, image.Height, image.Stride, image.ImageBytes);
            Defines.Native.YUV2RGB(ref yuv, ref rgb);
        }

        /// <summary>
        /// Downslales image by given factor efficiently 
        /// i.e multiplier 2 gives w/2,h/2.
        /// </summary>
        /// <param name="from"></param>
        /// <param name="to"></param>
        /// <param name="multiplier"></param>
        public static void Downscale(ImageData from, RgbImage to, int multiplier)
        {
            unsafe
            {
                if (from.isManaged)
                {
                    fixed (byte* dp = &from.data[from.dataOffset])
                    {
                        var ugi = new UnsafeGenericImage()
                        {
                            ImageBytes = dp,
                            Width = from.Width,
                            Height = from.Height,
                            Stride = from.Stride,
                            ImgType = from.ImgType,
                        };

                        var t = new UnsafeGenericImage();

                        t.ImageBytes = (byte*)to.ImageBytes;
                        t.Width = from.Width;
                        t.Height = from.Height;
                        t.Stride = from.Stride;
                        t.ImgType = ImageType.Rgb;

                        Defines.Native.DownscaleImg(ref ugi, ref t, multiplier);
                    }
                }
                else
                {

                    var ugi = new UnsafeGenericImage()
                    {
                        ImageBytes = (byte*)from.imageData.ToPointer(),
                        Width = from.Width,
                        Height = from.Height,
                        Stride = from.Stride,
                        ImgType = from.ImgType,
                    };

                    var t = new UnsafeGenericImage();

                    t.ImageBytes = (byte*)to.ImageBytes;
                    t.Width = from.Width;
                    t.Height = from.Height;
                    t.Stride = from.Stride;
                    t.ImgType = ImageType.Rgb;

                    Defines.Native.DownscaleImg(ref ugi, ref t, multiplier);
                    
                }
            }
        }


    }
}



