#ifndef LH_CSS_TRANSITION_H
#define LH_CSS_TRANSITION_H

#include "web_color.h"
#include "string_id.h"
#include <vector>
#include <cmath>

namespace litehtml
{
	// Timing function enum
	enum transition_timing
	{
		timing_linear = 0,
		timing_ease,
		timing_ease_in,
		timing_ease_out,
		timing_ease_in_out,
	};

	// Parsed from CSS `transition` property
	struct transition_spec
	{
		string_id	property = empty_id;	// _background_color_, _opacity_, or _all_ (via empty_id mapped to "all")
		float		duration_ms = 0;
		transition_timing timing = timing_ease;
		bool		is_all = false;			// transition: all ...

		bool matches(string_id prop) const
		{
			return is_all || property == prop;
		}
	};

	// Active transition for a single property on an element
	struct active_transition
	{
		string_id			property;
		float				duration_ms;
		transition_timing	timing;
		double				start_time = -1;	// -1 = not started, set on first tick

		// background-color
		web_color			old_color;
		web_color			new_color;

		// opacity
		float				old_opacity = 1.0f;
		float				new_opacity = 1.0f;

		// transform: scale
		float				old_scale = 1.0f;
		float				new_scale = 1.0f;
	};

	// Cubic bezier solver
	inline float cubic_bezier(float x1, float y1, float x2, float y2, float t)
	{
		if (t <= 0.f) return 0.f;
		if (t >= 1.f) return 1.f;

		float cx = 3.f * x1;
		float bx = 3.f * (x2 - x1) - cx;
		float ax = 1.f - cx - bx;
		float cy = 3.f * y1;
		float by = 3.f * (y2 - y1) - cy;
		float ay = 1.f - cy - by;

		auto sampleX = [&](float u) { return ((ax * u + bx) * u + cx) * u; };
		auto sampleY = [&](float u) { return ((ay * u + by) * u + cy) * u; };
		auto derivX  = [&](float u) { return (3.f * ax * u + 2.f * bx) * u + cx; };

		// Newton-Raphson + bisection fallback
		float u = t;
		for (int i = 0; i < 8; i++)
		{
			float err = sampleX(u) - t;
			if (std::abs(err) < 1e-6f) return sampleY(u);
			float d = derivX(u);
			if (std::abs(d) < 1e-6f) break;
			u -= err / d;
		}
		float lo = 0.f, hi = 1.f;
		for (int i = 0; i < 20; i++)
		{
			float mid = (lo + hi) * 0.5f;
			if (sampleX(mid) < t) lo = mid; else hi = mid;
		}
		return sampleY((lo + hi) * 0.5f);
	}

	inline float apply_easing(transition_timing timing, float t)
	{
		switch (timing)
		{
		case timing_linear:      return t;
		case timing_ease:        return cubic_bezier(0.25f, 0.1f, 0.25f, 1.0f, t);
		case timing_ease_in:     return cubic_bezier(0.42f, 0.0f, 1.0f, 1.0f, t);
		case timing_ease_out:    return cubic_bezier(0.0f, 0.0f, 0.58f, 1.0f, t);
		case timing_ease_in_out: return cubic_bezier(0.42f, 0.0f, 0.58f, 1.0f, t);
		default:                 return t;
		}
	}

	inline web_color lerp_color(const web_color& a, const web_color& b, float t)
	{
		return web_color(
			(byte)(a.red   + (int)((b.red   - a.red)   * t)),
			(byte)(a.green + (int)((b.green - a.green) * t)),
			(byte)(a.blue  + (int)((b.blue  - a.blue)  * t)),
			(byte)(a.alpha + (int)((b.alpha - a.alpha) * t))
		);
	}

	inline float lerp_float(float a, float b, float t)
	{
		return a + (b - a) * t;
	}
}

#endif // LH_CSS_TRANSITION_H
