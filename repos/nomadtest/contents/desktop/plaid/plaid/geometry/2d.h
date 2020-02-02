#ifndef PLAIDGADGET_GEOMETRY_H
#define PLAIDGADGET_GEOMETRY_H

#include <cmath>

#include "../core.h"


/*
    Geometry utility header: provides useful classes for working with
        cartesian geometry in two dimensions.
*/

namespace plaid
{
    class Point; //Alias Vector
    class Angle;
    class Scale;
    class Transform;
    class Line;


    //Useful constants
	const double PI_D = 3.14159265358979323846, TWO_PI_D = 2.0*PI_D;
	const float PI = 3.141594f, TWO_PI = 2.0f*PI;

	inline float DEGTORAD(float DEG) {return DEG*(PI/180.0f);}
	inline float RADTODEG(float RAD) {return RAD*(180.0f/PI);}


    //Fast inverse square root, thanks to the nutters at id software.
    inline float fInvSqrt(float _v)
    {
    	union {Sint32 i; float v;};
    	v = _v; i = 0x5f375a86 - (i >> 1); //insanity
    	return v * (1.5f - (.5f*_v*v*v)); //newtonian refinement or summat
    }


    /*
        Point models a two-dimensional point, complete with utility functions.
    */
    class Point : public Reflected
    {
    public:
        //Constructors
        inline Point()                   : x(0.0f),    y(0.0f)    {}
        inline Point(const Point &other) : x(other.x), y(other.y) {}
        inline Point(float _x, float _y) : x(_x),      y(_y)      {}

        //Destructor (Note, this makes it unsafe to extend this class)
        inline ~Point() {}

        //Assignment
        inline Point& operator=(const Point &other)
			{x = other.x; y = other.y; return *this;}

        //Comparison
        inline bool operator==(const Point &other) const
			{return (x == other.x && y == other.y);}
        inline bool operator!=(const Point &other) const
			{return (x != other.x || y != other.y);}

        //Arithmetic
        inline Point operator+() const
			{return *this;}
        inline Point operator+(const Point &other) const
			{return Point(x+other.x, y+other.y);}
        inline Point& operator+=(const Point &other)
			{x += other.x; y += other.y; return *this;}
        inline Point operator-() const
			{return Point(-x, -y);}
        inline Point operator-(const Point &other) const
			{return Point(x-other.x, y-other.y);}
        inline Point& operator-=(const Point &other)
			{x -= other.x; y -= other.y; return *this;}

        //Scaling
        inline Point operator*(float scale) const
			{return Point(x*scale, y*scale);}
		inline Point& operator*=(float scale)
			{x *= scale; y *= scale; return *this;}
        inline Point operator/(float scale) const
			{return Point(x/scale, y/scale);}
		inline Point& operator/=(float scale)
			{x /= scale; y /= scale; return *this;}

        //Magnitude and squared magnitude
        inline float magnitude() const {return std::sqrt((x*x) + (y*y));}
        inline float magSq()     const {return (x*x) + (y*y);}

        //Normalize the Point, unless its magnitude is zero.
        inline Point normalized() const
			{float m=magnitude(); m+=!m; return Point(x/m, y/m);}
        inline Point& normalize()
			{float m=magnitude(); m+=!m; x/=m; y/=m; return *this;}

#if PLAIDGADGET
	public:
		//Reflection info
		static Type _type; virtual Type type() const {return _type;}
#endif

    public:
        //Visible state
        float x, y;
    };

    typedef Point Vector;
    typedef Point Translation;


    //Dot product
	inline float Dot(const Point &a, const Point &b)
		{return (a.x*b.x + a.y*b.y);}

	//2D "Cross Product" (returns Z-value; X and Y are always 0)
	inline float Cross(const Point &a, const Point &b)
		{return a.x*b.y - b.x*a.y;}

	//Distance between two points
	inline float Distance(const Point &a, const Point &b)
		{return (b-a).magnitude();}


    /**
        Angle models an angle, or a rotation in two dimensions.
    */
    class Angle : public Reflected
    {
	public:
		//Constructors
        inline Angle() :
			vec(1.0f, 0.0f) {}
        inline Angle(const Angle &other) :
			vec(other.vec) {}
        inline Angle(float radians) :
			vec(cos(radians), sin(radians)) {}
        inline explicit Angle(const Point &p) :
			vec(p) {vec.x += ((!vec.x)&(!vec.y)); vec.normalize();}

        //Destructor (Note, this makes it unsafe to extend this class)
        inline ~Angle() {}

        //Assignment
        inline Angle& operator=(const Angle &other)
			{vec = other.vec; return *this;}
        inline Angle& operator=(float radians)
			{vec = Point(std::cos(radians), std::sin(radians)); return *this;}

        //Comparison
        inline bool operator==(const Angle &other) const
			{return (vec == other.vec);}
        inline bool operator!=(const Angle &other) const
			{return (vec != other.vec);}

        //Arithmetic
        inline Angle operator+() const
			{return Angle(vec, 0);}
        inline Angle operator+(const Angle &other) const
			{return Angle(other[vec], 0);}
        inline Angle operator+(float radians) const
			{return Angle(Angle(radians)[vec], 0);}
        inline Angle& operator+=(const Angle &other)
			{vec = other[vec]; return *this;}
        inline Angle& operator+=(float radians)
			{vec = Angle(radians)[vec]; return *this;}
        inline Angle operator-() const
			{return Angle(vec.x, -vec.y, 0);}
        inline Angle operator-(const Angle &other) const
			{return Angle((*this)[Point(other.vec.x, -other.vec.y)], 0);}
        inline Angle operator-(float radians) const
			{return Angle(Angle(-radians)[vec], 0);}
        inline Angle& operator-=(const Angle &other)
			{vec = (*this)[Point(other.vec.x, -other.vec.y)]; return *this;}
        inline Angle& operator-=(float radians)
			{vec = Angle(-radians)[vec]; return *this;}

        //Offset scaling
        /*inline Angle operator*(float factor) const
			{return Angle(get()*factor);}
        inline Angle& operator*=(float factor)
			{(*this) = (*this) * factor; return *this;}
        inline Angle operator/(float factor) const
			{return Angle(get()/factor);}
        inline Angle& operator/=(float factor)
			{(*this) = (*this) / factor; return *this;}*/

        //Rotation
        inline Point operator[](const Point &p) const
			{return Point(vec.x*p.x - vec.y*p.y, vec.y*p.x + vec.x*p.y);}

        inline float get() const      {return std::atan2(vec.y, vec.x);}
        inline void set(float a)      {vec.x=std::cos(a); vec.y=std::sin(a);}
		inline float degrees() const           {return RADTODEG(get());}
		inline void setDegrees(float degrees)  {return set(DEGTORAD(degrees));}

        //Conversions
        inline Point vector() const
			{return vec;}
        inline operator float() const
			{return get();}

        //Does this do anything?
		inline bool negligible() const
			{return (vec.x == 1.0f && vec.y == 0.0f);}

#if PLAIDGADGET
	public:
		//Reflection info
		static Type _type; virtual Type type() const {return _type;}
#endif

	private:
		Angle(const Point &_vec, int sig) : vec(_vec) {}
		Angle(float x, float y, int sig)  : vec(x, y) {}

    public:
        Point vec;
    };

	typedef Angle Rotation;

    //Degrees
    inline Angle Degrees(float degrees)
    {
    	return DEGTORAD(degrees);
    }

    //Direction
    inline Angle Direction(const Point &from, const Point &to)
    {
    	return Angle(to-from);
    }


    /**
        Scale: Represents two-dimensional scaling.
    */
    class Scale : public Reflected
    {
    public:
        //Constructors
        inline Scale()                    {scale = 1.0f;}
        inline Scale(float scale)         {this->scale = scale;}
        inline Scale(const Scale &other)  {scale = other.scale;}

        //Destructor (Note, this makes it unsafe to extend this class)
        inline ~Scale() {}

        //Assignment
        inline Scale& operator=(const Scale &o)  {scale=o.scale; return *this;}
        inline Scale& operator=(float s)         {scale=s; return *this;}

        //Comparison
        inline bool operator==(const Scale &other) const
			{return (scale == other.scale);}
        inline bool operator!=(const Scale &other) const
			{return (scale != other.scale);}

        //Inversion
        inline Scale operator~() const
			{return Scale(scale ? (1.0f / scale) : 0.0f);}

        //Scaling
        inline Point operator[](const Point &point) const
			{return Point(point.x * scale, point.y * scale);}

        //Scale scaling (?!)
        inline Scale operator*(const Scale &other) const
			{return Scale(scale * other.scale);}
        inline Scale& operator*=(const Scale &other)
			{scale *= other.scale; return *this;}
        inline Scale operator/(const Scale &other) const
			{return Scale(scale/other.scale);}
        inline Scale& operator/=(const Scale &other)
			{scale /= other.scale; return *this;}

		inline Scale operator*(float mult) const {return Scale(scale*mult);}
        inline Scale& operator*=(float mult)     {scale*=mult; return *this;}
        inline Scale operator/(float div) const  {return Scale(scale/div);}
        inline Scale& operator/=(float div)      {scale/=div; return *this;}

        //Conversion
        inline operator float() const   {return scale;}

		//Does this do anything?
		inline bool negligible() const  {return (scale == 1.0f);}

#if PLAIDGADGET
	public:
		//Reflection info
		static Type _type; virtual Type type() const {return _type;}
#endif

    public:
        //Visible state
        float scale;
    };


    /*
        Transform represents a two-dimensional transformation.
            It has three parts:

            r = rotation  (About the origin, before translation)
            s = scale  (About the origin, before translation)
            t = translation  (horizontal and vertical shift)
    */
    class Transform : public Reflected
    {
    public:
        //Constructors
        inline Transform()                                  {clearMatrix();}
        inline Transform(Point _t)           : t(_t)        {clearMatrix();}
        inline Transform(Angle _r)           : r(_r)        {clearMatrix();}
        inline Transform(Scale _s)           : s(_s)        {clearMatrix();}
        inline Transform(Point _t, Angle _r) : t(_t), r(_r) {clearMatrix();}
        inline Transform(Point _t, Scale _s) : t(_t), s(_s) {clearMatrix();}
        inline Transform(Angle _r, Scale _s) : r(_r), s(_s) {clearMatrix();}
        inline Transform(Point _t, Angle _r, Scale _s) : t(_t), r(_r), s(_s)
			{clearMatrix();}
		inline Transform(Point _t, Angle _r, Scale _s, const float *_m) :
			t(_t), r(_r), s(_s) {m[0]=_m[0];m[1]=_m[1];m[2]=_m[2];m[3]=_m[3];}
        inline Transform(const Transform &other)
        {
        	t = other.t;
            r = other.r;
            s = other.s;
            m[0] = other.m[0]; m[1] = other.m[1];
            m[2] = other.m[2]; m[3] = other.m[3];
        }
        inline Transform(const Transform &other, int signifies_inversion)
        {
            /*
                This constructor creates an inverted copy of a Transform.
                Notably, the order of operations can't be reversed, so we need
                    to use some trickery to make this work.
                Rotation and scaling are agnostic of one another but both can
                    interact with translation.  We adjust the latter for the
                    other two so it acts as though it was done first.
            */
            r = -other.r;
            s = ~other.s;
            t = r[s[-other.t]];
            float det = (other.m[0]*other.m[3] - other.m[1]*other.m[2]);
            if (det)
			{
				//Proper inversion is possible
				m[0] =  other.m[3] / det;
				m[1] = -other.m[1] / det;
				m[2] = -other.m[2] / det;
				m[3] =  other.m[0] / det;
			}
			else clearMatrix();
        }

        //Destructor (Note, this makes it unsafe to extend this class)
        inline ~Transform() {}

        static Transform translate(float x, float y)  {return Point(x, y);}
        static Transform translate(Point t)           {return t;}
        static Transform rotate(Angle r)              {return r;}
        static Transform rotateDegrees(float degrees) {return Degrees(degrees);}
        static Transform scale(Scale s)               {return s;}

        //Assignment
        inline void operator=(const Transform &other)
        {
            r = other.r; s = other.s; t = other.t;
            m[0] = other.m[0]; m[1] = other.m[1];
            m[2] = other.m[2]; m[3] = other.m[3];
        }

        //Comparison
        inline bool operator==(const Transform &other) const
        {
            return (t == other.t && r == other.r && s == other.s);
        }
        inline bool operator!=(const Transform &other) const
        {
            return (t != other.t || r != other.r || s != other.s);
        }

        //Application (moving things to the space of the transform)
        inline Point operator[](const Point &point) const
        {
        	Point p(point.x*m[0]+point.y*m[1], point.x*m[2]+point.y*m[3]);
            return s[r[p]] + t;
        }
        inline Point pointOut(const Point &point) const
        {
        	Point p(point.x*m[0]+point.y*m[1], point.x*m[2]+point.y*m[3]);
        	return s[r[p]] + t;
        }
        inline Point vectorOut(const Point &vec) const
        {
        	Point p(vec.x*m[0]+vec.y*m[1], vec.x*m[2]+vec.y*m[3]);
        	return s[r[p]];
        }
        inline Point pointIn(const Point &point) const
        {
        	//return (~s)[(-r)[point - t]];
        	return (~*this).pointOut(point);
        }
        inline Point vectorIn(const Point &vec) const
        {
        	//return (~s)[(-r)[point]];
        	return (~*this).vectorOut(vec);
        }

        /*
			Use this operator for concatenating transformations.  A copy of the
				parameter, in the space of the parent transform, will be
				returned.

			(operator[] is an alias to operator() for consistency with scripts)
        */
        inline Transform operator()(const Transform &original) const
		{
			if (irregular() || original.irregular())
			{
				//We might need to do math on the irregularity matrix
				Transform t((*this)[original.t], r+original.r, s*original.s);
				float tmp[4];const float *om=original.m;Point rp=original.r.vec;
				t.m[0]= om[0]*rp.x +om[1]*rp.y; t.m[1]=-om[0]*rp.y +om[1]*rp.x;
				t.m[2]= om[2]*rp.x +om[3]*rp.y; t.m[3]=-om[2]*rp.y +om[3]*rp.x;
				tmp[0]=t.m[0]*m[0]+t.m[1]*m[2]; tmp[1]=t.m[0]*m[1]+t.m[1]*m[3];
				tmp[2]=t.m[2]*m[0]+t.m[3]*m[2]; tmp[3]=t.m[2]*m[1]+t.m[3]*m[3];
				t.m[0]=tmp[0]*rp.x-tmp[1]*rp.y; t.m[1]=tmp[1]*rp.x+tmp[0]*rp.y;
				t.m[2]=tmp[2]*rp.x-tmp[3]*rp.y; t.m[3]=tmp[3]*rp.x+tmp[2]*rp.y;
				return t;
			}
			else return Transform(
				(*this)[original.t],
				r+original.r,
				s*original.s);
		}
        inline Transform operator[](const Transform &original) const
			{return (*this)(original);}

		//Inversion
        inline Transform operator~() const
        {
            return Transform(*this, 0);
        }

        //Find transform from this one to another, the cheap way
        inline Transform to(const Transform &other)
        {
        	return other(~(*this));
        }

        //Interpolation and difference between transforms
        inline Transform lerp(const Transform &other, float value)
        {
        	float neg = (1.0f - value);
        	return Transform(t*neg + other.t*value,
				r + float(other.r-r) * value,
				std::pow(s, neg) * std::pow(other.s, value));
        }
        inline Transform operator+(const Transform &other) const
        {
        	return Transform(t + other.t, r + other.r, s * other.s);
        }
        inline Transform &operator+=(const Transform &other)
        {
        	t += other.t; /* THE DIFFERENCE BETWEEN ADDITION AND */
        	r += other.r; /*   CONCATENATION OF TRANSFORMS IS TO */
        	s *= other.s; /*   BE STRESSED.  ADDITION IS FOR     */
        	return *this; /*   INTERPOLATION ONLY!  See above.   */
        }
        inline Transform operator-(const Transform &other) const
        {
        	return Transform(t - other.t, r - other.r, s / other.s);
        }
        inline Transform &operator-=(const Transform &other)
        {
        	t -= other.t;
        	r -= other.r;
        	s /= other.s;
        	return *this;
        }
        inline Transform operator*(float scale) const
        {
        	return Transform(t * scale, float(r) * scale, std::pow(s, scale));
        }
        inline Transform& operator*=(float scale)
        {
        	t *= scale;
        	r = float(r)*scale;
        	s = std::pow(s, scale);
        	return *this;
        }

        //Addition / subtraction of components
        inline Transform operator+(const Point &p) const
			{return Transform(t+p, r, s, m);}
        inline Transform& operator+=(const Point &p)
			{t += p; return *this;}
        inline Transform operator-(const Point &p) const
			{return Transform(t-p, r, s, m);}
        inline Transform& operator-=(const Point &p)
			{t -= p; return *this;}
        inline Transform operator+(const Angle &a) const
			{return Transform(t, r+a, s, m);}
        inline Transform& operator+=(const Angle &a)
			{r += a; return *this;}
        inline Transform operator-(const Angle &a) const
			{return Transform(t, r-a, s, m);}
        inline Transform& operator-=(const Angle &a)
			{r -= a; return *this;}

        /*
			And-ing of components
        */
        inline Transform operator&(const Point &p) const
			{return Transform(t+p, r, s, m);}
        inline Transform& operator&=(const Point &p)
			{t += p; return *this;}
        inline Transform operator&(const Angle &a) const
			{return Transform(t, r+a, s, m);}
        inline Transform& operator&=(const Angle &a)
			{r += a; return *this;}
        inline Transform operator&(const Scale &c) const
			{return Transform(t, r, s*c, m);}
        inline Transform& operator&=(const Scale &c)
			{s *= c; return *this;}

        //Matrix manipulation

        //Resets the irregular-transform matrix to identity.
        void clearMatrix()
        {
        	m[0] = 1.0f; m[1] = 0.0f;
        	m[2] = 0.0f; m[3] = 1.0f;
        }

        //Returns true if the irregularity matrix is NOT a unit transform.
        bool irregular() const
        {
        	return (m[0]!=1.f)|(m[1]!=.0f)|(m[2]!=.0f)|(m[3]!=1.f);
        }

        //Defines an irregular X/Y scaling; useful for flipping.
        void stretch(float xFact, float yFact)
        {
        	m[0] = xFact; m[1] = 0.0f;
        	m[2] = 0.0f; m[3] = yFact;
        }

        //Defines an irregular scale in a given direction.
        void stretch(Angle a, float scale)
        {
        	Point in = a.vec;
        	m[0] = (scale*in.x*in.x + in.y*in.y);
        	m[1] = (scale*in.x*in.y - in.x*in.y);
			m[2] = (scale*in.x*in.y - in.x*in.y);
			m[3] = (scale*in.y*in.y + in.x*in.x);
        }

#if PLAIDGADGET
	public:
		//Reflection info
		static Type _type; virtual Type type() const {return _type;}
#endif

    public:
        //Visible state
        Point t;
        Angle r;
        Scale s;

        //Advanced:  A 2D transform matrix for skews and irregular scaling.
        //  Applied before rotation/scaling factors.
        float m[4];
    };


    /*
		A simple rectangle.
    */
    class Rectangle : public Reflected
    {
	public:
		Rectangle() {}
		Rectangle(Point pos) :
			a(pos), b(pos) {}
		Rectangle(Point _a, Point _b) :
			a(_a), b(_b) {}
		Rectangle(float x1, float y1, float x2, float y2) :
			a(x1, y1), b(x2, y2) {}

		float width() const
			{return b.x - a.x;}
		float height() const
			{return b.y - a.y;}

		Point center() const
			{return (a+b)*.5f;}
		float radius() const
			{return .5f*std::max(std::abs(a.x-b.x), std::abs(a.y-b.y));}

#if PLAIDGADGET
	public:
		//Reflection info
		static Type _type; virtual Type type() const {return _type;}
#endif

	public:
		Point a, b;
    };


	/*
		Represents a line between two points.
	*/
    class Line : public Reflected
    {
    public:
        inline Line()
        {
        }
        inline Line(const Point &pointA, const Point &pointB) :
            a(pointA), b(pointB)
        {
        }

        inline float length() const
        {
            //return (b-a).magnitude(); is cleaner but probably less efficient
            return std::sqrt( (b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y) );
        }

        inline bool vertical() const
        {
            return (a.x == b.x);
        }
        inline bool horizontal() const
        {
            return (a.y == b.y);
        }
        inline bool point() const
        {
            return (a.x == b.x && a.y == b.y);
        }
        inline float slope() const
        {
            return (vertical() ? 0.0f : ((b.y - a.y) / (b.x - a.x)));
        }
        inline float lean() const
        {
            return (horizontal() ? 0.0f : ((b.x - a.x) / (b.y - a.y)));
        }
        inline float x_intercept() const
        {
            //y = mx + b.  b = y - mx.
            return (horizontal() ? 0.0f :
                (a.x - a.y*((b.x - a.x) / (b.y - a.y))));
        }
        inline float y_intercept() const
        {
            //y = mx + b.  b = y - mx.
            return (vertical() ? 0.0f :
                (a.y - a.x*((b.y - a.y) / (b.x - a.x))));
        }

        inline bool intersects(const Line &other, Point &location) const
        {
            if (vertical())
            {
                if (other.vertical())
                {
                    if (std::max(a.y, b.y) >= std::min(other.a.y, other.b.y) &&
                        std::min(a.y, b.y) <= std::max(other.a.y, other.b.y))
                    {
                        location.x = a.x;
                        location.y = std::max(std::min(a.y, b.y),
                            std::min(other.a.y, other.b.y));
                        return true;
                    }
                    return false;
                }
                else
                {
                    float y = other.y_intercept() + (a.x * other.slope());
                    if (y >= std::min(a.y, b.y) && y <= std::max(a.y, b.y))
                    {
                        location.x = a.x;
                        location.y = y;
                        return true;
                    }
                    return false;
                }
            }
            else
            {
                if (other.vertical())
                {
                    float y = y_intercept() + (other.a.x * slope());
                    if (y >= std::min(other.a.y, other.b.y) &&
                        y <= std::max(other.a.y, other.b.y))
                    {
                        location.x = other.a.x;
                        location.y = y;
                        return true;
                    }
                    return false;
                }
                else
                {
                    // Y1 = M1*X + B1
                    // Y2 = M2*X + B2
                    // Y1 = Y2
                    // M1*X + B1 = M2*X + B2
                    // (M1-M2)*X = B2-B1
                    // X = (B2-B1) / (M1-M2)
                    float m1 = slope(), m2 = other.slope(),
                        b1 = y_intercept(), b2 = other.y_intercept();
                    float x = (b2-b1) / (m1-m2);
                    if (x >= std::max(std::min(a.x, b.x),
                            std::min(other.a.x, other.b.x)) &&
                        x <= std::min(std::max(a.x, b.x),
                            std::max(other.a.x, other.b.x)) )
                    {
                        location.x = x;
                        location.y = m1*x + b1;
                        return true;
                    }
                    return false;
                }
            }
        }
        inline bool intersects(const Line &other) const
        {
            Point p;
            return intersects(other, p);
        }

#if PLAIDGADGET
	public:
		//Reflection info
		static Type _type; virtual Type type() const {return _type;}
#endif

	public:
        Point a, b;
    };


    /*
		More operator overloads, where they make sense.
			(Notably, transform components can be and-ed together.)
	*/
	inline Transform operator&(const Point &p, const Angle &a)
		{return Transform(p, a);}
	inline Transform operator&(const Angle &a, const Point &p)
		{return Transform(p, a);}
	inline Transform operator&(const Point &p, const Scale &s)
		{return Transform(p, s);}
	inline Transform operator&(const Scale &s, const Point &p)
		{return Transform(p, s);}
	inline Transform operator&(const Angle &a, const Scale &s)
		{return Transform(a, s);}
	inline Transform operator&(const Scale &s, const Angle &a)
		{return Transform(a, s);}

	inline Point operator*(const float &scale, const Point &p) {return p*scale;}
	inline Angle operator*(const float &scale, const Angle &a) {return a*scale;}
	inline Scale operator*(const float &scale, const Scale &s) {return s*scale;}
	inline Transform operator*(const float &scale, const Transform &t)
		{return t*scale;}
}


#endif // PLAIDGADGET_GEOMETRY_H
