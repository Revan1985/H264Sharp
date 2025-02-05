﻿using System;
using System.Threading;

namespace H264Sharp
{
    /// <summary>
    /// H264 Encoder based on Cisco's OpenH264
    /// </summary>
    public class H264Encoder : IDisposable
    {
        private readonly IntPtr encoder;
        private bool disposedValue;
        private int disposed = 0;
        private static bool enableDebugPrints = false;
        private NativeBindings native => Defines.Native;

        /// <summary>
        /// Enables Debug prints of initialization.
        /// </summary>
        public static bool EnableDebugPrints { set => EnableDebug(value); get => enableDebugPrints; }

        private static void EnableDebug(bool value)
        {
            enableDebugPrints = value;
            Defines.Native.EncoderEnableDebugLogs(value ? 1 : 0);
        }

        /// <summary>
        /// Creates new instance. You can change the cisco dll name with <see cref="Defines"></see> class before initialisation
        /// </summary>
        public H264Encoder()
        {
            encoder = native.GetEncoder(Defines.CiscoDllName);
        }

        /// <summary>
        /// Creates new instance. 
        /// </summary>
        public H264Encoder(string ciscoDllPath)
        {
            encoder = native.GetEncoder(ciscoDllPath);
        }

        /// <summary>
        /// Gets default advanced configuration parameters
        /// </summary>
        /// <returns></returns>
        public TagEncParamExt GetDefaultParameters()
        {
            TagEncParamExt paramExt = new TagEncParamExt();
            native.GetDefaultParams(encoder, ref paramExt);

            return paramExt;
        }

        /// <summary>
        /// Initialises the encoder.
        /// </summary>
        /// <param name="width">Expected frame width</param>
        /// <param name="height">Expected frame height</param>
        /// <param name="bitrate">Target bitrate in bits</param>
        /// <param name="fps">Target frames per second</param>
        /// <param name="configType">Configuration type</param>
        public int Initialize(int width, int height, int bitrate, int fps, ConfigType configType)
        {
            return native.InitializeEncoder(encoder, width, height, bitrate, fps, (int)configType);

        }

        /// <summary>
        ///  Initialises the encoder.
        /// </summary>
        /// <param name="param"></param>
        /// <returns></returns>
        public int Initialize(TagEncParamBase param)
        {
            return native.InitializeEncoderBase(encoder, param);
        }

        /// <summary>
        ///  Initialises the encoder.
        /// </summary>
        /// <param name="param"></param>
        /// <returns></returns>
        public int Initialize(TagEncParamExt param)
        {
            return native.InitializeEncoder2(encoder, param);
        }

        /// <summary>
        /// Forces an intra frame with instant decoder refresh on next encode.
        /// This is used to refresh decoder in case of lost frames.
        /// Be aware that Inta frames are large.
        /// </summary>
        /// <returns></returns>
        public bool ForceIntraFrame()
        {
            var ret = native.ForceIntraFrame(encoder);
            return ret == 0 ? true : false;
        }

        /// <summary>
        /// Sets the target max bitrate.
        /// you can call this at any point
        /// </summary>
        /// <param name="target"></param>
        public void SetMaxBitrate(int target)
        {
            native.SetMaxBitrate(encoder, target);
        }

        /// <summary>
        /// Sets target fps.
        /// you can call this at any point.
        /// </summary>
        /// <param name="target"></param>
        public void SetTargetFps(float target)
        {
            native.SetTargetFps(encoder, target);
        }

        /// <summary>
        /// Gets an option
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="option"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        public bool GetOption<T>(ENCODER_OPTION option, out T value) where T : struct
        {
            unsafe
            {

                value = new T();
                if (value is bool)
                {
                    bool v = (bool)((object)value);
                    byte toSet = v ? (byte)1 : (byte)0;
                    {
                        int r = native.GetOptionEncoder(encoder, option, new IntPtr(&toSet));
                        var success = (r == 0);
                        value = (T)(object)(toSet == 1 ? true : false);
                        return success;
                    }
                }
                fixed (T* V = &value)
                {
                    int r = native.GetOptionEncoder(encoder, option, new IntPtr(V));
                    value = *V;
                    return (r == 0);
                }
            }
        }

        /// <summary>
        /// Gets an option, allows reuse of the value
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="option"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        public bool GetOptionRef<T>(ENCODER_OPTION option, ref T value) where T : struct
        {
            unsafe
            {
                fixed (T* V = &value)
                {
                    int r = native.GetOptionEncoder(encoder, option, new IntPtr(V));
                    value = *V;
                    return (r == 0);
                }
            }
        }

        /// <summary>
        /// Sets an option
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="option"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        public bool SetOption<T>(ENCODER_OPTION option, T value) where T : struct
        {
            unsafe
            {
                if (value is bool)
                {
                    bool v = (bool)((object)value);
                    byte toSet = v ? (byte)1 : (byte)0;
                    return native.SetOptionEncoder(encoder, option, new IntPtr(&toSet)) == 0;
                }
                return native.SetOptionEncoder(encoder, option, new IntPtr(&value)) == 0;
            }
        }

        /// <summary>
        /// Encodes imagee provided on Image data.
        /// </summary>
        /// <param name="im"></param>
        /// <param name="ed"></param>
        /// <returns></returns>
        public bool Encode(ImageData im, out EncodedData[] ed)
        {
            unsafe
            {
                var fc = new FrameContainer();
                if (im.isManaged)
                {
                    fixed (byte* dp = &im.data[im.dataOffset])
                    {
                        var ugi = new UnsafeGenericImage()
                        {
                            ImageBytes = dp,
                            Width = im.Width,
                            Height = im.Height,
                            Stride = im.Stride,
                            ImgType = im.ImgType,
                        };


                        var success = native.Encode(encoder, ref ugi, ref fc);
                        ed = Convert(fc);
                        return success == 1;
                    }
                }
                else
                {

                    var ugi = new UnsafeGenericImage()
                    {
                        ImageBytes = (byte*)im.imageData.ToPointer(),
                        Width = im.Width,
                        Height = im.Height,
                        Stride = im.Stride,
                        ImgType = im.ImgType,
                    };

                    var success = native.Encode(encoder, ref ugi, ref fc);
                    ed = Convert(fc);
                    return success == 1;

                }

            }
        }

        /// <summary>
        /// Encodes Yuv402P images
        /// </summary>
        /// <param name="YUV">start pointer</param>
        /// <param name="ed"></param>
        /// <returns></returns>
        public unsafe bool Encode(byte* YUV, out EncodedData[] ed)
        {
            var fc = new FrameContainer();
            var success = native.Encode1(encoder, ref YUV[0], ref fc);
            ed = Convert(fc);
            return success == 1;
        }

        /// <summary>
        /// Encodes Yuv402P images
        /// </summary>
        /// <param name="yuv"></param>
        /// <param name="ed"></param>
        /// <returns></returns>
        public bool Encode(YUVImagePointer yuv, out EncodedData[] ed)
        {
            unsafe { return Encode(yuv.Y, out ed); }
        }

        /// <summary>
        /// Encodes Yuv402P images
        /// </summary>
        /// <param name="yuv"></param>
        /// <param name="ed"></param>
        /// <returns></returns>
        public bool Encode(YuvImage yuv, out EncodedData[] ed)
        {
            unsafe { return Encode(((byte*)yuv.ImageBytes.ToPointer()), out ed); }
        }


        private unsafe EncodedData[] Convert(FrameContainer fc)
        {
            EncodedData[] data = new EncodedData[fc.FrameCount];
            for (int i = 0; i < fc.FrameCount; i++)
            {
                var ef = *fc.Frames;
                data[i] = new EncodedData(ef);

                fc.Frames++;
            }
            return data;
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (Interlocked.CompareExchange(ref disposed, 1, 0) == 0)
                    native.FreeEncoder(encoder);

                disposedValue = true;
            }
        }

        ~H264Encoder()
        {
            Dispose(disposing: false);
        }

        public void Dispose()
        {
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }

       
    }


    public enum ConfigType
    {
        /// <summary>
        /// Standard setting for camera capture.
        /// <br/>This is recommended option for camera capture
        /// </summary>
        CameraBasic,
        /// <summary>
        /// Standard setting for screen capture
        /// <br/>This is recommended option for screen capture
        /// </summary>
        ScreenCaptureBasic,
        /// <summary>
        /// Advanced configuration camera capture.
        /// Uses LTR references, 0 intra, and high profile
        /// </summary>
        CameraCaptureAdvanced,
        /// <summary>
        /// Advanced configuration screen capture
        /// </summary>
        ScreenCaptureAdvanced
    };

}
