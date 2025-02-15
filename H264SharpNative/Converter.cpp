#include "Converter.h"
#include "ThreadPool.h"
#include "Yuv2Rgb.h"
#include "Rgb2Yuv.h"
namespace H264Sharp {

    ConverterConfig Converter::Config;
    // now we can expose all possible formats

    template<int NUM_CH, bool RGB>
    void Converter::Yuv420PtoRGB(unsigned char* dst_ptr,
        const unsigned char* y_ptr,
        const unsigned char* u_ptr,
        const unsigned char* v_ptr,
        signed   int   width,
        signed   int   height,
        signed   int   y_span,
        signed   int   uv_span,
        signed   int   dst_span)
    {
    
        int numThreads = Converter::Config.Numthreads;
        numThreads = width * height < Converter::minSize ? 1 : numThreads;
#ifndef __arm__

        int enableSSE = Converter::Config.EnableSSE;
        int enableAvx2 = Converter::Config.EnableAvx2;

        if (enableAvx2>0 && width % 32 == 0)
        {
            Yuv2Rgb::ConvertYUVToRGB_AVX2<NUM_CH, RGB>(width,
                height,
                y_ptr,
                u_ptr,
                v_ptr,
                y_span,
                uv_span,
                dst_ptr,
                dst_span,
                numThreads);
        }
        else if (enableSSE > 0 && width % 16 == 0)
        {
           
            // SSE, may parallel, not arm
            Yuv2Rgb::yuv420_rgb24_sse<NUM_CH, RGB>(width,
                height,
                y_ptr,
                u_ptr,
                v_ptr,
                y_span,
                uv_span,
                dst_ptr,
                dst_span,
                numThreads);
            return;
        }
        else
        {

            Yuv2Rgb::Yuv420P2RGBDefault<NUM_CH, RGB>(dst_ptr,
                y_ptr,
                u_ptr,
                v_ptr,
                width,
                height,
                y_span,
                uv_span,
                dst_span,
                numThreads);
        }

    #elif defined(__aarch64__)
        int enableNeon = Converter::Config.EnableNeon;

        if (enableNeon > 0 && width % 16 == 0)
        {
               
                    Yuv2Rgb::ConvertYUVToRGB_NEON<NUM_CH, RGB>(
                        y_ptr,
                        u_ptr,
                        v_ptr,
                        y_span,
                        uv_span,
                        dst_ptr,
                        width,
                        height,
                        numThreads);
            } 
            else 
            {
                Yuv2Rgb::Yuv420P2RGBDefault<NUM_CH, RGB>(dst_ptr,
                    y_ptr,
                    u_ptr,
                    v_ptr,
                    width,
                    height,
                    y_span,
                    uv_span,
                    dst_span,
                    numThreads);
            }
                
    #else
            Yuv2Rgb::Yuv420P2RGBDefault<NUM_CH, RGB>(dst_ptr,
                y_ptr,
                u_ptr,
                v_ptr,
                width,
                height,
                y_span,
                uv_span,
                dst_span,
                numThreads);
    #endif
    }

    #pragma endregion



   /* void Converter::Yuv420PtoRGB(unsigned char* dst_ptr, const unsigned char* y_ptr,
                             const unsigned char* u_ptr,const unsigned char* v_ptr, 
                             signed int width, signed int height,
                             signed int y_span, signed int uv_span, signed int dst_span)
    {
        Yuv420PtoRGB_t<3, true>(dst_ptr,
            y_ptr,
            u_ptr,
            v_ptr,
            width,
            height,
            y_span,
            uv_span,
            dst_span);
    }*/

   

    template <int NUM_CH, bool IS_RGB>
    void Converter::RGBXtoYUV420Planar(unsigned char* bgra, unsigned char* dst, int width, int height, int stride)
    {
        int numThreads = Converter::Config.Numthreads;
        numThreads = width * height < minSize ? 1 : numThreads;

#ifdef __arm__
#if defined(__aarch64__)
        int enableNeon = Converter::Config.EnableNeon;
        if (enableNeon > 0 && width % 16 == 0)
            Rgb2Yuv::RGBtoYUV420PlanarNeon<NUM_CH, IS_RGB>(bgra, dst, width, height, stride, numThreads);
        else
            Rgb2Yuv::RGBXtoYUV420Planar<NUM_CH, IS_RGB>(bgra, dst, width, height, stride, numThreads);
#else
        Rgb2Yuv::RGBXtoYUV420Planar<NUM_CH, IS_RGB>(bgra, dst, width, height, stride, numThreads);
#endif
#else

        int enableAVX2 = Converter::Config.EnableAvx2;
        if (enableAVX2 > 0 && width % 32 == 0)
            Rgb2Yuv::RGBXToI420_AVX2<NUM_CH, IS_RGB>(bgra, dst, width, height, stride, numThreads);
        else
            Rgb2Yuv::RGBXtoYUV420Planar<NUM_CH, IS_RGB>(bgra, dst, width, height, stride, numThreads);

#endif

    }

    template void Converter::RGBXtoYUV420Planar<4, true>(unsigned char* bgra, unsigned char* dst, int width, int height, int stride);
    template void Converter::RGBXtoYUV420Planar<4, false>(unsigned char* bgra, unsigned char* dst, int width, int height, int stride);
    template void Converter::RGBXtoYUV420Planar<3, true>(unsigned char* bgra, unsigned char* dst, int width, int height, int stride);
    template void Converter::RGBXtoYUV420Planar<3, false>(unsigned char* bgra, unsigned char* dst, int width, int height, int stride);

    //-------------------------------Downscale------------------------------------------------
    /*
    * __m256i indices = _mm256_setr_epi32(0, 6, 12, 18, 24, 30, 36, 42); // Indices to gather

	__m256i result = _mm256_i32gather_epi32((int*)rgb, indices, 1);   // Scale = 4 (sizeof(int))
	result = _mm256_shuffle_epi8(result , _mm256_setr_epi8(0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1,
														   0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1));
	result = _mm256_permutevar8x32_epi32(result, _mm256_setr_epi32(0,1,2,4,5,6,7,3));
    */
    void Converter::Downscale24(unsigned char* RESTRICT rgbSrc, int width, int height, int stride, unsigned char* RESTRICT dst, int multiplier)
    {
        int index = 0;
        int dinx = 0;
        for (int i = 0; i < height / multiplier; i++)
        {
    #pragma clang loop vectorize(assume_safety)
            for (int j = 0; j < width / multiplier; j++)
            {

                dst[dinx++] = rgbSrc[index];
                dst[dinx++] = rgbSrc[index + 1];
                dst[dinx++] = rgbSrc[index + 2];

                index += 3 * multiplier;
            }
            index = stride * multiplier * (i + 1);
        }
    }

    void Converter::Downscale32(unsigned char* RESTRICT rgbaSrc, int width, int height, int stride, unsigned char* RESTRICT dst, int multiplier)
    {
        int index = 0;
        int dinx = 0;
        uint32_t* RESTRICT src = (uint32_t*)rgbaSrc;
        uint32_t* RESTRICT dst1 = (uint32_t*)dst;
        for (int i = 0; i < height / multiplier; i++)
        {
    #pragma clang loop vectorize(assume_safety)

            for (int j = 0; j < width / multiplier; j++)
            {
                dst1[dinx++] = src[index];
                index += multiplier;
            }
            index = stride * multiplier * (i + 1);
        }
    }

    template  void Converter::Yuv420PtoRGB<3, true>(unsigned char* dst_ptr,
        const unsigned char* y_ptr,
        const unsigned char* u_ptr,
        const unsigned char* v_ptr,
        signed   int   width,
        signed   int   height,
        signed   int   y_span,
        signed   int   uv_span,
        signed   int   dst_span);
}


