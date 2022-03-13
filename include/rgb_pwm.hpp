#pragma once
#include <Arduino.h>
#include <gfx_core.hpp>
#include <gfx_pixel.hpp>
#include <gfx_positioning.hpp>
namespace arduino {
    template<uint8_t Pin,uint8_t Channel, uint32_t FrequencyHz=5000,size_t BitDepth=8,unsigned int MaxPercent=100> struct pwm_traits {
        static_assert(BitDepth>0,"BitDepth must be at least 1");
        static_assert(BitDepth<=16,"BitDepth must be at most 16");
        static_assert(FrequencyHz>0,"FrequencyHz must not be zero");
        static_assert(MaxPercent>0,"MaxPercent must not be zero");
        static_assert(MaxPercent<=100,"MaxPercent must not be more than 100");
        constexpr static const uint8_t pin = Pin;
        constexpr static const uint8_t channel = Channel & 15;
        constexpr static const uint32_t frequency_hz = FrequencyHz;
        constexpr static const size_t bit_depth = BitDepth;
        constexpr static const float max_scale = MaxPercent/100.0;
        
        static void initialize() {
            ledcSetup(channel,frequency_hz,bit_depth);
            ledcAttachPin(pin,channel);
        }
    };

    // controls up to 5 RGB led PWM channels, presenting a draw target with a width of N leds
    template<typename RTraits,typename GTraits, typename BTraits>
    struct rgb_pwm_group {
        static_assert(RTraits::channel!=GTraits::channel,"The channels must be unique - R and G");
        static_assert(RTraits::channel!=BTraits::channel,"The channels must be unique - R and B");
        static_assert(GTraits::channel!=BTraits::channel,"The channels must be unique - G and B");
        static_assert(RTraits::pin!=GTraits::pin,"The pins must be unique - R and G");
        static_assert(RTraits::pin!=BTraits::pin,"The pins must be unique - R and B");
        static_assert(GTraits::pin!=BTraits::pin,"The pins must be unique - G and B");
        using R = RTraits;
        using G = GTraits;
        using B = BTraits;
        using R_channel = gfx::channel_traits<gfx::channel_name::R,R::bit_depth>;
        using G_channel = gfx::channel_traits<gfx::channel_name::G,G::bit_depth>;
        using B_channel = gfx::channel_traits<gfx::channel_name::B,B::bit_depth>;
        using pixel_type = gfx::pixel<R_channel,G_channel,B_channel>;
        using R_int_type = typename pixel_type::template channel_by_name<gfx::channel_name::R>::int_type;
        using G_int_type = typename pixel_type::template channel_by_name<gfx::channel_name::G>::int_type;
        using B_int_type = typename pixel_type::template channel_by_name<gfx::channel_name::B>::int_type;
        
        static inline void initialize() {
            R::initialize();
            G::initialize();
            B::initialize();
        }
        static inline R_int_type read_R() {
            return (R_int_type)(ledcRead(R::channel)*R::max_scale);
        }
        static inline G_int_type read_G() {
            return (G_int_type)(ledcRead(G::channel)*G::max_scale);
        }
        static inline B_int_type read_B() {
            return (R_int_type)(ledcRead(B::channel)*B::max_scale);
        }
        static inline void write_R(R_int_type value) {
            ledcWrite(R::channel,gfx::helpers::clamp(value,R_int_type(0),R_channel::max));
        }
        static inline void write_G(G_int_type value) {
            ledcWrite(G::channel,gfx::helpers::clamp(value,G_int_type(0),G_channel::max));
        }
        static inline void write_B(B_int_type value) {
            ledcWrite(B::channel,gfx::helpers::clamp(value,B_int_type(0),B_channel::max));
        }
        static inline void write_pixel(pixel_type color) {
            write_R(color.template channel<gfx::channel_name::R>());
            write_G(color.template channel<gfx::channel_name::G>());
            write_B(color.template channel<gfx::channel_name::B>());
        }
        static inline void read_pixel(pixel_type* out_color) {
            if(!out_color) return;
            out_color->template channel<gfx::channel_name::R>(read_R());
            out_color->template channel<gfx::channel_name::G>(read_G());
            out_color->template channel<gfx::channel_name::B>(read_B());
        }
    };
    namespace rgb_pwm_helpers {
        template<typename... RgbPwmGroups> struct rgb_pwm_groups_initializer {
            static inline void initialize() {
            }
        };
        template<typename RgbPwmGroup,typename... RgbPwmGroups> struct rgb_pwm_groups_initializer<RgbPwmGroup,RgbPwmGroups...> {
            using next = rgb_pwm_groups_initializer<RgbPwmGroups...>;
            static inline void initialize() {
                RgbPwmGroup::initialize();
                next::initialize();
            }
        };
        template<size_t Index,typename... RgbPwmGroups> struct rgb_pwm_fetch_group {
            // HACK: this makes it compile
            using group = rgb_pwm_fetch_group;
            using pixel_type = gfx::rgb_pixel<16>;
            static void write_pixel(pixel_type color) {

            }
            static void read_pixel(pixel_type* out_color) {
                
            }
        };
        template<size_t Index,typename RgbPwmGroup,typename... RgbPwmGroups> struct rgb_pwm_fetch_group<Index,RgbPwmGroup,RgbPwmGroups...> {
            using group = typename rgb_pwm_fetch_group<Index-1,RgbPwmGroups...>::group;
        };
        template<typename RgbPwmGroup,typename... RgbPwmGroups> struct rgb_pwm_fetch_group<0,RgbPwmGroup,RgbPwmGroups...> {
            using group = RgbPwmGroup;
        };
    }
    template<typename... RgbPwmGroups>
    struct rgb_pwm {
        using pixel_type = gfx::rgb_pixel<24>;
        using caps = gfx::gfx_caps<false,false,false,false,false,true,false>;
        constexpr static const uint16_t width = (uint16_t)sizeof...(RgbPwmGroups);
        constexpr static const uint16_t height = 1;
    private:
        bool m_initialized;
        void set_pixel(int group,pixel_type color) {
            using g0 = typename rgb_pwm_helpers::rgb_pwm_fetch_group<0,RgbPwmGroups...>::group;
            using g1 = typename rgb_pwm_helpers::rgb_pwm_fetch_group<1,RgbPwmGroups...>::group;
            using g2 = typename rgb_pwm_helpers::rgb_pwm_fetch_group<2,RgbPwmGroups...>::group;
            using g3 = typename rgb_pwm_helpers::rgb_pwm_fetch_group<3,RgbPwmGroups...>::group;
            using g4 = typename rgb_pwm_helpers::rgb_pwm_fetch_group<4,RgbPwmGroups...>::group;
            typename g0::pixel_type px0;
            typename g1::pixel_type px1;
            typename g2::pixel_type px2;
            typename g3::pixel_type px3;
            typename g4::pixel_type px4;
            switch(group%5) {
                case 0:
                    gfx::convert(color,&px0);
                    g0::write_pixel(px0);
                    break;
                case 1:
                    gfx::convert(color,&px1);
                    g1::write_pixel(px1);
                    break;
                case 2:
                    gfx::convert(color,&px2);
                    g2::write_pixel(px2);
                    break;
                case 3:
                    gfx::convert(color,&px3);
                    g3::write_pixel(px3);
                    break;
                case 4:
                    gfx::convert(color,&px4);
                    g4::write_pixel(px4);
                    break;
            }
        }
        template<int Group>
        void get_pixel_helper(pixel_type* out_color) {
            using g = typename rgb_pwm_helpers::rgb_pwm_fetch_group<Group,RgbPwmGroups...>::group;
            typename g::pixel_type px;
            g::read_pixel(&px);
            gfx::convert(px,out_color);
        }
        void get_pixel(int group,pixel_type* out_color) {
            if(!out_color) { return; }
            switch(group%5) {
                case 0:
                    get_pixel_helper<0>(out_color);
                    break;
                case 1:
                    get_pixel_helper<1>(out_color);
                    break;
                case 2:
                    get_pixel_helper<2>(out_color);
                    break;
                case 3:
                    get_pixel_helper<3>(out_color);
                    break;
                case 4:
                    get_pixel_helper<4>(out_color);
                    break;
            }
        }
    public:
        rgb_pwm() : m_initialized(false) {

        }
        gfx::gfx_result initialize() {
            if(!m_initialized) {
                rgb_pwm_helpers::rgb_pwm_groups_initializer<RgbPwmGroups...>::initialize();
                m_initialized=true;
            }
            return gfx::gfx_result::success;
        }
        
        inline bool initialized() const { return m_initialized; }
        inline gfx::size16 dimensions() const { return {width,height}; };
        inline gfx::rect16 bounds() const { return dimensions().bounds(); }
        inline gfx::gfx_result point(gfx::point16 location, pixel_type color) {
            gfx::gfx_result r = initialize();
            if(r!=gfx::gfx_result::success) {
                return r;
            }
            if(location.y<0 || location.y>=height) {
                return gfx::gfx_result::success;
            }
            set_pixel(location.x,color);
            return gfx::gfx_result::success;
        }
        gfx::gfx_result fill(const gfx::rect16& bounds,pixel_type color) {
            gfx::gfx_result r = initialize();
            if(r!=gfx::gfx_result::success) {
                return r;
            }
            const gfx::rect16 rr = bounds.crop(this->bounds()).normalize();
            for(int i = rr.x1;i<=rr.x2;++i) {
                set_pixel(i,color);
            }
            return gfx::gfx_result::success;
        }
        inline gfx::gfx_result clear(const gfx::rect16& bounds) { 
            pixel_type px;
            return fill(bounds,px);
        }
        gfx::gfx_result point(gfx::point16 location,pixel_type* out_color) {
            if(!initialized()) {
                return gfx::gfx_result::invalid_state;
            }
            if(location.y<0 || location.y>=height) {
                return gfx::gfx_result::success;
            }
            get_pixel(location.x,out_color);
            return gfx::gfx_result::success;
        }
    };
}