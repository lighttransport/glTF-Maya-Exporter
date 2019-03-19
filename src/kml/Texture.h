#pragma once
#ifndef _KML_TEXTURE_H_
#define _KML_TEXTURE_H_

#include <algorithm>
#include <array>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace kml
{
    class ColorBalance
    {
    public:
        ColorBalance() : m_exposure(0.0f),
                         m_defaultColor({0.5f, 0.5f, 0.5f}),
                         m_colorGain({0.5f, 0.5f, 0.5f}),
                         m_colorOffset({0.0f, 0.0f, 0.0f}),
                         m_alphaGain(1.0f),
                         m_alphaOffset(0.0f),
                         m_alphaIsLuminance(false){};

        ~ColorBalance(){};

        ColorBalance(const ColorBalance& rh)
        {
            m_exposure = rh.m_exposure;
            m_defaultColor = rh.m_defaultColor;
            m_colorGain = rh.m_colorGain;
            m_colorOffset = rh.m_colorOffset;
            m_alphaGain = rh.m_alphaGain;
            m_alphaOffset = rh.m_alphaOffset;
            m_alphaIsLuminance = rh.m_alphaIsLuminance;
        }

        ColorBalance& operator=(const ColorBalance& rh)
        {
            m_exposure = rh.m_exposure;
            m_defaultColor = rh.m_defaultColor;
            m_colorGain = rh.m_colorGain;
            m_colorOffset = rh.m_colorOffset;
            m_alphaGain = rh.m_alphaGain;
            m_alphaOffset = rh.m_alphaOffset;
            m_alphaIsLuminance = rh.m_alphaIsLuminance;

            return (*this);
        }

        float m_exposure;                    // EV
        std::array<float, 3> m_defaultColor; // in linear space RGB
        std::array<float, 3> m_colorGain;    // in linear space RGB
        std::array<float, 3> m_colorOffset;  // in linear space RGB
        float m_alphaGain;
        float m_alphaOffset;
        bool m_alphaIsLuminance;
    };

    class Texture
    {
    public:
        enum FilterType
        {
            FILTER_NEAREST = 0,
            FILTER_LINEAR
        };
        enum WrapType
        {
            WRAP_REPEAT = 0,
            WRAP_CLAMP,
            WRAP_MIRROR
        };

    public:
        Texture() : m_colorSpace("sRGB"),
                    m_repeatU(0.0f), m_repeatV(0.0f),
                    m_offsetU(0.0f), m_offsetV(0.0f),
                    m_wrapU(WRAP_REPEAT),
                    m_wrapV(WRAP_REPEAT),
                    m_filterU(FILTER_NEAREST),
                    m_filterV(FILTER_NEAREST),
                    m_udimmode(false),
                    m_exist(true){};

        ~Texture(){};

        Texture(const Texture& rh)
        {
            m_textureFilePath = rh.m_textureFilePath;
            m_cacheTextureFilePath = rh.m_cacheTextureFilePath;
            m_colorSpace = rh.m_colorSpace;
            m_repeatU = rh.m_repeatU;
            m_repeatV = rh.m_repeatV;
            m_offsetU = rh.m_offsetU;
            m_offsetV = rh.m_offsetV;
            m_wrapU = rh.m_wrapU;
            m_wrapV = rh.m_wrapV;
            m_filterU = rh.m_filterU;
            m_filterV = rh.m_filterV;
            m_udimmode = rh.m_udimmode;
            m_udimIDs = rh.m_udimIDs;

            m_colorBalance = rh.m_colorBalance;
        }

        Texture* clone()
        {
            return new Texture(*this);
        }

        void SetFilePath(const std::string& filePath)
        {
            m_textureFilePath = filePath;
        }

        std::string GetFilePath() const
        {
            return m_textureFilePath;
        }

        void SetCacheFilePath(const std::string& filePath)
        {
            m_cacheTextureFilePath = filePath;
        }

        std::string GetCacheFilePath() const
        {
            return m_cacheTextureFilePath;
        }

        void SetUDIMFilePath(const std::string& filePath)
        {
            m_udimTextureFilePath = filePath;
        }

        std::string GetUDIMFilePath() const
        {
            return m_udimTextureFilePath;
        }

        std::string MakeUDIMFilePath(int udimID) const
        {
            std::string path = m_udimTextureFilePath;
            std::stringstream strID;
            strID << udimID;
            const std::string udimStr = "<UDIM>";
            size_t p = path.find(udimStr);
            if (p != std::string::npos)
            {
                path.replace(p, udimStr.size(), strID.str());
            }
            return path;
        }

        void SetRepeat(float repeatU, float repeatV)
        {
            m_repeatU = repeatU;
            m_repeatV = repeatV;
        }
        float GetRepeatU() const { return m_repeatU; }
        float GetRepeatV() const { return m_repeatV; }

        void SetOffset(float offsetU, float offsetV)
        {
            m_offsetU = offsetU;
            m_offsetV = offsetV;
        }

        float GetOffsetU() const { return m_offsetU; }
        float GetOffsetV() const { return m_offsetV; }

        void SetWrap(bool wrapU, bool wrapV)
        {
            if (wrapU)
            {
                m_wrapU = WRAP_REPEAT;
            }
            else
            {
                m_wrapU = WRAP_CLAMP;
            }
            if (wrapV)
            {
                m_wrapV = WRAP_REPEAT;
            }
            else
            {
                m_wrapV = WRAP_CLAMP;
            }
        }

        bool GetWrapU() const { return m_wrapU != WRAP_CLAMP; }
        bool GetWrapV() const { return m_wrapV != WRAP_CLAMP; }

        void SetWrapTypeU(WrapType wrap)
        {
            m_wrapU = wrap;
        }

        void SetWrapTypeV(WrapType wrap)
        {
            m_wrapV = wrap;
        }

        WrapType GetWrapTypeU() const
        {
            return m_wrapU;
        }

        WrapType GetWrapTypeV() const
        {
            return m_wrapV;
        }

        void SetFilter(FilterType filter)
        {
            m_filterU = filter;
            m_filterV = filter;
        }

        void SetFilterTypeU(FilterType filter)
        {
            m_filterU = filter;
        }

        void SetFilterTypeV(FilterType filter)
        {
            m_filterV = filter;
        }

        FilterType GetFilter() const
        {
            return m_filterU;
        }

        FilterType GetFilterTypeU() const
        {
            return m_filterU;
        }

        FilterType GetFilterTypeV() const
        {
            return m_filterV;
        }

        void SetUDIMMode(bool enable)
        {
            m_udimmode = enable;
        }

        bool GetUDIMMode() const
        {
            return m_udimmode;
        }

        void AddUDIM_ID(int udim_id)
        {
            m_udimIDs.push_back(udim_id);
        }

        void ClearUDIM_ID()
        {
            m_udimIDs.clear();
        }
        const std::vector<int> GetUDIM_IDs() const
        {
            return m_udimIDs;
        }

        bool FileExists() const
        {
            return m_exist;
        }
        void SetFileExists(bool bExist)
        {
            m_exist = bExist;
        }

        void SetColorSpace(const std::string& colorSpace)
        {
            m_colorSpace = colorSpace;
        }

        const std::string& GetColorSpace() const
        {
            return m_colorSpace;
        }

        void SetColorBalance(const ColorBalance& colorBalance)
        {
            m_colorBalance = colorBalance;
        }

        const ColorBalance GetColorBalance() const
        {
            return m_colorBalance;
        }

    protected:
        std::string m_textureFilePath;
        std::string m_cacheTextureFilePath;
        std::string m_udimTextureFilePath;
        std::string m_colorSpace; // sRGB, Raw, ...
        float m_repeatU, m_repeatV;
        float m_offsetU, m_offsetV;
        WrapType m_wrapU, m_wrapV;
        FilterType m_filterU, m_filterV;
        bool m_udimmode;
        std::vector<int> m_udimIDs;
        bool m_exist;

        ColorBalance m_colorBalance;
    };
} // namespace kml

#endif
