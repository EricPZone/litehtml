# litehtml — fork with performance optimizations and CSS features

This is a fork of [litehtml](https://github.com/litehtml/litehtml), the lightweight HTML/CSS rendering engine.

The upstream engine is well designed and easy to integrate, but it lacks CSS transitions, transform support, and has performance bottlenecks when documents are frequently recreated or when pseudo-class changes trigger unnecessary work. This fork addresses these issues.

## Motivation

The original litehtml engine recomputes all CSS selectors on every pseudo-class change (`:hover`, `:active`), re-parses CSS on every document creation, and allocates heap memory during the draw loop. For interactive use cases with frequent hover events and document refreshes, these costs add up. This fork reduces per-frame overhead and adds missing CSS features (transitions, transforms) to enable smooth, animated UI rendering.

## Changes from upstream

### CSS transitions

Full CSS transition system with support for `background-color`, `opacity`, and `transform` properties.

- `transition_spec` parses the CSS `transition` shorthand (property, duration, timing function)
- `active_transition` tracks interpolation state per element per property
- `document::tick(timestamp)` drives all active transitions from an external frame loop
- Cubic-bezier timing function solver (Newton-Raphson with bisection fallback) supporting `ease`, `ease-in`, `ease-out`, `ease-in-out`, and `linear`
- Mid-transition reversal: when a property changes direction mid-animation, the new transition starts from the current interpolated value (not the original start value)
- Transitions are created in `find_styles_changes()` by comparing snapshots taken before and after style recomputation

**Files:** `include/litehtml/css_transition.h` (new), `src/element.cpp`, `src/document.cpp`

### CSS transform: scale()

Parsing and rendering support for `transform: scale(X)` (number or percentage).

- Parsed from `CV_FUNCTION` tokens in `style.cpp`, stored as `float m_transform_scale` on `css_properties`
- Applied at draw time in `html_tag::draw()` and `el_image::draw()` — post-layout, no reflow needed
- Container callbacks `set_transform(scale, cx, cy)` / `reset_transform()` allow the rendering backend to apply the transform around the element's center point

**Files:** `src/style.cpp`, `include/litehtml/css_properties.h`, `src/html_tag.cpp`, `src/el_image.cpp`, `include/litehtml/document_container.h`

### Opacity callbacks

Container callbacks `set_opacity(float)` / `reset_opacity()` called during element rendering, enabling the backend to apply opacity as a GPU-level operation rather than per-pixel blending.

**Files:** `include/litehtml/document_container.h`, `src/html_tag.cpp`

### Element click with data-action

`on_element_click()` callback on `document_container` enables click handling on any element via `data-action` attributes, not just `<a>` anchors. The click event bubbles up the DOM tree until an element with `data-action` is found.

**Files:** `include/litehtml/document_container.h`

### Performance optimizations

#### Dirty flag for pseudo-class changes (Opt 6)

`m_styles_dirty` flag on `element` avoids iterating all CSS selectors on every `requires_styles_update()` call. When a pseudo-class changes (`:hover`, `:active`), the flag is set on the element and propagated recursively to all descendants (needed for composite selectors like `td:hover img`). The flag is checked first in `requires_styles_update()` — O(1) vs O(N) selector iteration.

**Files:** `include/litehtml/element.h`, `src/element.cpp`, `src/html_tag.cpp`

#### Non-recursive compute_styles in find_styles_changes (Opt 6b)

`find_styles_changes()` calls `compute_styles(false)` (non-recursive) instead of the default recursive version. Since `find_styles_changes()` already recurses into children, the recursive `compute_styles()` would prematurely recompute children's CSS values, destroying the old-value snapshots needed for transition detection.

**Files:** `src/element.cpp`

#### CSS caching across document recreations

`css::append_from()` merges one stylesheet into another. Combined with a pre-parsed CSS cache (`LitehtmlCssCache` in the WASM bindings), CSS is parsed once and reused across multiple document creations via `document::createFromString(html, container, cached_css)`.

**Files:** `include/litehtml/stylesheet.h`, `src/document.cpp`

#### Single-pass stylesheet application 

User CSS is merged into the document stylesheet with `append_from()` before applying. One tree walk instead of two.

**Files:** `include/litehtml/stylesheet.h`, `src/document.cpp`

#### Zero-allocation draw_stacking_context

Replaced `std::map<int, bool>` with a stack-local `int[32]` array + `std::sort()` for z-index collection during stacking context rendering. Eliminates per-frame heap allocation.

**Files:** `src/render_item.cpp`

#### calc_outlines dirty tracking

Early return in `calc_outlines()` when `parent_width` hasn't changed since the last call. Avoids redundant percentage-to-pixel calculations for padding, margins, and borders.

**Files:** `include/litehtml/render_item.h`, `src/render_item.cpp`

## Original README

**litehtml** is the lightweight HTML rendering engine with CSS2/CSS3 support. Note that **litehtml** itself does not draw any text, pictures or other graphics and that **litehtml** does not depend on any image/draw/font library. You are free to use any library to draw images, fonts and any other graphics. **litehtml** just parses HTML/CSS and places the HTML elements into the correct positions (renders HTML). To draw the HTML elements you have to implement the simple callback interface [document_container](https://github.com/litehtml/litehtml/wiki/document_container).

### HTML Parser

**litehtml** uses the [gumbo-parser](https://codeberg.org/gumbo-parser/gumbo-parser) to parse HTML. Gumbo is an implementation of the HTML5 parsing algorithm implemented as a pure C99 library with no outside dependencies.

### License

**litehtml** is distributed under [New BSD License](https://opensource.org/licenses/BSD-3-Clause).
The **gumbo-parser** is distributed under [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0)

### Links

  * [upstream source code](https://github.com/litehtml/litehtml)
  * [website](http://www.litehtml.com/)
