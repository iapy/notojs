struct SVGPathElement : bridge::Interface<SVGPathElement, dom::SVGElement, SVGGraphicsElement>
{
    SVGPathElement(JSContext *ctx, JSValue self) : Base{ctx, self} {}
    SVGPathElement(std::reference_wrapper<dom::HTMLElement> &&rw) : Base(std::move(rw)) {}

    struct Point
    {
        double x = 0;
        double y = 0;
    };

    struct Segment
    {
        Point a;
        Point b;
        double length = 0;
    };

    struct Path
    {
        std::vector<Segment> segments;
        double total = 0;
    };

    static void skip(char const *&p)
    {
        while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') ++p;
    }

    static bool command(char c)
    {
        return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
    }

    static bool supported(char c)
    {
        switch(c)
        {
        case 'M': case 'm':
        case 'L': case 'l':
        case 'H': case 'h':
        case 'V': case 'v':
        case 'C': case 'c':
        case 'S': case 's':
        case 'Q': case 'q':
        case 'T': case 't':
        case 'A': case 'a':
        case 'Z': case 'z':
            return true;
        default:
            return false;
        }
    }

    static bool number(char const *&p, double &n)
    {
        skip(p);
        if(command(*p) || *p == '\0') return false;

        char *end = nullptr;
        n = std::strtod(p, &end);
        if(end == p) return false;

        p = end;
        skip(p);
        return true;
    }

    static JSValue invalid(JSContext *ctx)
    {
        JS_ThrowTypeError(ctx, "Invalid SVG path data");
        return JS_EXCEPTION;
    }

    static JSValue unsupported(JSContext *ctx)
    {
        JS_ThrowTypeError(ctx, "Unsupported SVG path command");
        return JS_EXCEPTION;
    }

    static Point relative(bool const relative, Point current, double x, double y)
    {
        return relative ? Point{current.x + x, current.y + y} : Point{x, y};
    }

    static Point reflect(Point current, Point control)
    {
        return {2 * current.x - control.x, 2 * current.y - control.y};
    }

    static void line(Path &path, Point from, Point to)
    {
        double const length = std::hypot(to.x - from.x, to.y - from.y);
        path.segments.push_back({from, to, length});
        path.total += length;
    }

    static Point quadratic(Point p0, Point p1, Point p2, double t)
    {
        double const u = 1 - t;
        return {
            u * u * p0.x + 2 * u * t * p1.x + t * t * p2.x,
            u * u * p0.y + 2 * u * t * p1.y + t * t * p2.y
        };
    }

    static Point cubic(Point p0, Point p1, Point p2, Point p3, double t)
    {
        double const u = 1 - t;
        return {
            u * u * u * p0.x + 3 * u * u * t * p1.x + 3 * u * t * t * p2.x + t * t * t * p3.x,
            u * u * u * p0.y + 3 * u * u * t * p1.y + 3 * u * t * t * p2.y + t * t * t * p3.y
        };
    }

    static void quadratic(Path &path, Point p0, Point p1, Point p2)
    {
        Point previous = p0;
        for(std::size_t i = 1; i <= 32; ++i)
        {
            Point const next = quadratic(p0, p1, p2, static_cast<double>(i) / 32);
            line(path, previous, next);
            previous = next;
        }
    }

    static void cubic(Path &path, Point p0, Point p1, Point p2, Point p3)
    {
        Point previous = p0;
        for(std::size_t i = 1; i <= 32; ++i)
        {
            Point const next = cubic(p0, p1, p2, p3, static_cast<double>(i) / 32);
            line(path, previous, next);
            previous = next;
        }
    }

    static double angle(Point u, Point v)
    {
        return std::atan2(u.x * v.y - u.y * v.x, u.x * v.x + u.y * v.y);
    }

    static void arc(Path &path, Point p0, double rx, double ry, double x_axis_rotation, bool large_arc, bool sweep, Point p1)
    {
        if(p0.x == p1.x && p0.y == p1.y) return;

        rx = std::abs(rx);
        ry = std::abs(ry);
        if(rx == 0 || ry == 0)
        {
            line(path, p0, p1);
            return;
        }

        double const pi = std::acos(-1);
        double const phi = x_axis_rotation * pi / 180;
        double const cos_phi = std::cos(phi);
        double const sin_phi = std::sin(phi);
        double const dx = (p0.x - p1.x) / 2;
        double const dy = (p0.y - p1.y) / 2;
        double const x1p = cos_phi * dx + sin_phi * dy;
        double const y1p = -sin_phi * dx + cos_phi * dy;
        double const x1p2 = x1p * x1p;
        double const y1p2 = y1p * y1p;

        double radii = x1p2 / (rx * rx) + y1p2 / (ry * ry);
        if(radii > 1)
        {
            double const scale = std::sqrt(radii);
            rx *= scale;
            ry *= scale;
        }

        double const rx2 = rx * rx;
        double const ry2 = ry * ry;
        double const numerator = rx2 * ry2 - rx2 * y1p2 - ry2 * x1p2;
        double const denominator = rx2 * y1p2 + ry2 * x1p2;
        double const ratio = denominator == 0 ? 0 : numerator / denominator;
        double const factor = std::sqrt(ratio < 0 ? 0 : ratio);
        double const sign = large_arc == sweep ? -1 : 1;
        double const cxp = sign * factor * rx * y1p / ry;
        double const cyp = -sign * factor * ry * x1p / rx;
        double const cx = cos_phi * cxp - sin_phi * cyp + (p0.x + p1.x) / 2;
        double const cy = sin_phi * cxp + cos_phi * cyp + (p0.y + p1.y) / 2;

        Point const u{(x1p - cxp) / rx, (y1p - cyp) / ry};
        Point const v{(-x1p - cxp) / rx, (-y1p - cyp) / ry};
        double theta = angle({1, 0}, u);
        double delta = angle(u, v);
        if(!sweep && delta > 0)
            delta -= 2 * pi;
        else if(sweep && delta < 0)
            delta += 2 * pi;

        auto const n = static_cast<std::size_t>(std::ceil(std::abs(delta) / (pi / 16)));
        std::size_t const steps = n ? n : 1;
        Point previous = p0;
        for(std::size_t i = 1; i <= steps; ++i)
        {
            double const t = theta + delta * static_cast<double>(i) / steps;
            double const x = cx + rx * std::cos(t) * cos_phi - ry * std::sin(t) * sin_phi;
            double const y = cy + rx * std::cos(t) * sin_phi + ry * std::sin(t) * cos_phi;
            Point const next = i == steps ? p1 : Point{x, y};
            line(path, previous, next);
            previous = next;
        }
    }

    JSValue parse(JSContext *ctx, Path &path) const
    {
        auto attr = ref().getAttribute({"d"});
        if(!attr) return JS_UNDEFINED;

        std::string data{*attr};
        char const *p = data.c_str();
        char cmd = 0;
        Point current;
        Point start;
        Point cubicControl;
        Point quadraticControl;
        bool haveCurrent = false;
        bool cubicControlValid = false;
        bool quadraticControlValid = false;

        auto reset = [&] {
            cubicControlValid = false;
            quadraticControlValid = false;
        };

        skip(p);
        while(*p)
        {
            if(command(*p))
            {
                cmd = *p++;
                if(!supported(cmd)) return unsupported(ctx);
                skip(p);
            }
            else if(!cmd)
                return invalid(ctx);

            bool const rel = 'a' <= cmd && cmd <= 'z';

            switch(cmd)
            {
            case 'M':
            case 'm':
            {
                double x;
                double y;
                if(!number(p, x) || !number(p, y)) return invalid(ctx);

                current = rel && haveCurrent ? Point{current.x + x, current.y + y} : Point{x, y};
                start = current;
                haveCurrent = true;
                reset();

                while(*p && !command(*p))
                {
                    if(!number(p, x) || !number(p, y)) return invalid(ctx);
                    Point next = relative(rel, current, x, y);
                    line(path, current, next);
                    current = next;
                    reset();
                }

                cmd = rel ? 'l' : 'L';
                break;
            }
            case 'L':
            case 'l':
            {
                if(!haveCurrent) return invalid(ctx);

                double x;
                double y;
                if(!number(p, x) || !number(p, y)) return invalid(ctx);

                Point next = relative(rel, current, x, y);
                line(path, current, next);
                current = next;
                reset();

                while(*p && !command(*p))
                {
                    if(!number(p, x) || !number(p, y)) return invalid(ctx);
                    next = relative(rel, current, x, y);
                    line(path, current, next);
                    current = next;
                    reset();
                }
                break;
            }
            case 'H':
            case 'h':
            {
                if(!haveCurrent) return invalid(ctx);

                double x;
                if(!number(p, x)) return invalid(ctx);

                Point next = rel ? Point{current.x + x, current.y} : Point{x, current.y};
                line(path, current, next);
                current = next;
                reset();

                while(*p && !command(*p))
                {
                    if(!number(p, x)) return invalid(ctx);
                    next = rel ? Point{current.x + x, current.y} : Point{x, current.y};
                    line(path, current, next);
                    current = next;
                    reset();
                }
                break;
            }
            case 'V':
            case 'v':
            {
                if(!haveCurrent) return invalid(ctx);

                double y;
                if(!number(p, y)) return invalid(ctx);

                Point next = rel ? Point{current.x, current.y + y} : Point{current.x, y};
                line(path, current, next);
                current = next;
                reset();

                while(*p && !command(*p))
                {
                    if(!number(p, y)) return invalid(ctx);
                    next = rel ? Point{current.x, current.y + y} : Point{current.x, y};
                    line(path, current, next);
                    current = next;
                    reset();
                }
                break;
            }
            case 'C':
            case 'c':
            {
                if(!haveCurrent) return invalid(ctx);

                double x1, y1, x2, y2, x, y;
                if(!number(p, x1) || !number(p, y1) || !number(p, x2) || !number(p, y2) || !number(p, x) || !number(p, y)) return invalid(ctx);

                Point c1 = relative(rel, current, x1, y1);
                Point c2 = relative(rel, current, x2, y2);
                Point next = relative(rel, current, x, y);
                cubic(path, current, c1, c2, next);
                current = next;
                cubicControl = c2;
                cubicControlValid = true;
                quadraticControlValid = false;

                while(*p && !command(*p))
                {
                    if(!number(p, x1) || !number(p, y1) || !number(p, x2) || !number(p, y2) || !number(p, x) || !number(p, y)) return invalid(ctx);
                    c1 = relative(rel, current, x1, y1);
                    c2 = relative(rel, current, x2, y2);
                    next = relative(rel, current, x, y);
                    cubic(path, current, c1, c2, next);
                    current = next;
                    cubicControl = c2;
                    cubicControlValid = true;
                    quadraticControlValid = false;
                }
                break;
            }
            case 'S':
            case 's':
            {
                if(!haveCurrent) return invalid(ctx);

                double x2, y2, x, y;
                if(!number(p, x2) || !number(p, y2) || !number(p, x) || !number(p, y)) return invalid(ctx);

                Point c1 = cubicControlValid ? reflect(current, cubicControl) : current;
                Point c2 = relative(rel, current, x2, y2);
                Point next = relative(rel, current, x, y);
                cubic(path, current, c1, c2, next);
                current = next;
                cubicControl = c2;
                cubicControlValid = true;
                quadraticControlValid = false;

                while(*p && !command(*p))
                {
                    if(!number(p, x2) || !number(p, y2) || !number(p, x) || !number(p, y)) return invalid(ctx);
                    c1 = reflect(current, cubicControl);
                    c2 = relative(rel, current, x2, y2);
                    next = relative(rel, current, x, y);
                    cubic(path, current, c1, c2, next);
                    current = next;
                    cubicControl = c2;
                    cubicControlValid = true;
                    quadraticControlValid = false;
                }
                break;
            }
            case 'Q':
            case 'q':
            {
                if(!haveCurrent) return invalid(ctx);

                double x1, y1, x, y;
                if(!number(p, x1) || !number(p, y1) || !number(p, x) || !number(p, y)) return invalid(ctx);

                Point c1 = relative(rel, current, x1, y1);
                Point next = relative(rel, current, x, y);
                quadratic(path, current, c1, next);
                current = next;
                quadraticControl = c1;
                quadraticControlValid = true;
                cubicControlValid = false;

                while(*p && !command(*p))
                {
                    if(!number(p, x1) || !number(p, y1) || !number(p, x) || !number(p, y)) return invalid(ctx);
                    c1 = relative(rel, current, x1, y1);
                    next = relative(rel, current, x, y);
                    quadratic(path, current, c1, next);
                    current = next;
                    quadraticControl = c1;
                    quadraticControlValid = true;
                    cubicControlValid = false;
                }
                break;
            }
            case 'T':
            case 't':
            {
                if(!haveCurrent) return invalid(ctx);

                double x, y;
                if(!number(p, x) || !number(p, y)) return invalid(ctx);

                Point c1 = quadraticControlValid ? reflect(current, quadraticControl) : current;
                Point next = relative(rel, current, x, y);
                quadratic(path, current, c1, next);
                current = next;
                quadraticControl = c1;
                quadraticControlValid = true;
                cubicControlValid = false;

                while(*p && !command(*p))
                {
                    if(!number(p, x) || !number(p, y)) return invalid(ctx);
                    c1 = reflect(current, quadraticControl);
                    next = relative(rel, current, x, y);
                    quadratic(path, current, c1, next);
                    current = next;
                    quadraticControl = c1;
                    quadraticControlValid = true;
                    cubicControlValid = false;
                }
                break;
            }
            case 'A':
            case 'a':
            {
                if(!haveCurrent) return invalid(ctx);

                double rx, ry, x_axis_rotation, large_arc_flag, sweep_flag, x, y;
                if(!number(p, rx) || !number(p, ry) || !number(p, x_axis_rotation) || !number(p, large_arc_flag) || !number(p, sweep_flag) || !number(p, x) || !number(p, y)) return invalid(ctx);

                Point next = relative(rel, current, x, y);
                arc(path, current, rx, ry, x_axis_rotation, large_arc_flag != 0, sweep_flag != 0, next);
                current = next;
                reset();

                while(*p && !command(*p))
                {
                    if(!number(p, rx) || !number(p, ry) || !number(p, x_axis_rotation) || !number(p, large_arc_flag) || !number(p, sweep_flag) || !number(p, x) || !number(p, y)) return invalid(ctx);
                    next = relative(rel, current, x, y);
                    arc(path, current, rx, ry, x_axis_rotation, large_arc_flag != 0, sweep_flag != 0, next);
                    current = next;
                    reset();
                }
                break;
            }
            case 'Z':
            case 'z':
                if(!haveCurrent) return invalid(ctx);
                line(path, current, start);
                current = start;
                reset();
                cmd = 0;
                break;
            default:
                return unsupported(ctx);
            }

            skip(p);
        }

        return JS_UNDEFINED;
    }

    JSValue pathLength(JSContext *ctx) const
    {
        if(auto value = ref().getAttribute({"pathLength"}))
            return JS_NewFloat64(ctx, std::atof(value->data()));
        return JS_NewFloat64(ctx, 0);
    }

    void set_pathLength(JSContext *, bridge::Number n)
    {
        auto const s = std::to_string(n.as_double());
        ref().setAttribute({"pathLength"}, {s.c_str(), s.size()});
    }

    JSValue getTotalLength(JSContext *ctx) const
    {
        Path path;
        if(auto result = parse(ctx, path); JS_IsException(result))
            return result;
        return JS_NewFloat64(ctx, path.total);
    }

    JSValue getPointAtLength(JSContext *ctx, bridge::Number distance) const
    {
        Path path;
        if(auto result = parse(ctx, path); JS_IsException(result))
            return result;

        double d = distance.as_double();
        if(d < 0) d = 0;
        if(d > path.total) d = path.total;

        Point point;
        if(!path.segments.empty())
        {
            double offset = 0;
            for(auto const &segment : path.segments)
            {
                if(d <= offset + segment.length)
                {
                    double const t = segment.length ? (d - offset) / segment.length : 0;
                    point = {
                        segment.a.x + (segment.b.x - segment.a.x) * t,
                        segment.a.y + (segment.b.y - segment.a.y) * t
                    };
                    break;
                }

                offset += segment.length;
                point = segment.b;
            }
        }

        bridge::Object obj{ctx};
        obj.set("x", JS_NewFloat64(ctx, point.x));
        obj.set("y", JS_NewFloat64(ctx, point.y));
        return obj;
    }

    using Base::Base;
    using ctor = bridge::Unconstructable<SVGPathElement>;
    static JSCFunctionListEntry const funcs[];
};

JSCFunctionListEntry const SVGPathElement::funcs[] = {
    JS_CGETSET_DEF("pathLength", &bridge::Getter<&SVGPathElement::pathLength>, &bridge::Setter<&SVGPathElement::set_pathLength>),
    JS_CFUNC_DEF("getTotalLength", 0, &bridge::Function<&SVGPathElement::getTotalLength>::invoke),
    JS_CFUNC_DEF("getPointAtLength", 1, &bridge::Function<&SVGPathElement::getPointAtLength>::invoke),
};
