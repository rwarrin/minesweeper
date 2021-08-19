#ifndef MINESWEEPER_MATH_H

/**
 * SCALAR OPERATIONS
 **/

inline s32
Clamp(s32 Min, s32 Value, s32 Max)
{
    s32 Result = Value;
    if(Value < Min)
    {
        Result = Min;
    }
    else if(Value > Max)
    {
        Result = Max;
    }
    return(Result);
}

inline f32
Clamp(f32 Min, f32 Value, f32 Max)
{
    f32 Result = Value;
    if(Value < Min)
    {
        Result = Min;
    }
    else if(Value > Max)
    {
        Result = Max;
    }
    return(Result);
}

inline f32
Clamp01(f32 Value)
{
    f32 Result = Clamp(0.0f, Value, 1.0f);
    return(Result);
}

inline f32
Clamp01MapToRange(f32 Min, f32 t, f32 Max)
{
    f32 Result = 0.0f;

    f32 Range = Max - Min;
    if(Range != 0.0f)
    {
        Result = Clamp01((t - Min) / Range);
    }

    return(Result);
}

inline f32
Lerp(f32 A, f32 t, f32 B)
{
    f32 Result = (1.0f - t)*A + t*B;
    return(Result);
}

inline s32
Absolute(s32 Value)
{
    s32 Result = Value;
    if(Result < 0)
    {
        Result = -Result;
    }
    return(Result);
}

inline f32
Ceil(f32 Value)
{
    f32 Result = ceil(Value);
    return(Result);
}

/**
 * V2 Operations
 **/
union v2
{
    struct
    {
        f32 x, y;
    };
    f32 E[2];
};

inline v2
V2(f32 A, f32 B)
{
    v2 Result = {};
    Result.x = A;
    Result.y = B;
    return(Result);
}

inline v2
operator +(v2 A, v2 B)
{
    v2 Result = {};
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    return(Result);
}

inline v2&
operator +=(v2 &A, v2 B)
{
    A.x = A.x + B.x;
    A.y = A.y + B.y;
    return(A);
}

inline v2
operator -(v2 A, v2 B)
{
    v2 Result = {};
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    return(Result);
}

inline v2&
operator -=(v2 &A, v2 B)
{
    A.x = A.x - B.x;
    A.y = A.y - B.y;
    return(A);
}

inline v2
operator *(v2 A, f32 B)
{
    v2 Result = {};
    Result.x = A.x*B;
    Result.y = A.y*B;
    return(Result);
}

inline v2
operator *(f32 A, v2 B)
{
    v2 Result = B*A;
    return(Result);
}

inline v2
Hadamard(v2 A, v2 B)
{
    v2 Result = {};
    Result.x = A.x*B.x;
    Result.y = A.y*B.y;
    return(Result);
}

inline f32
Inner(v2 A, v2 B)
{
    f32 Result = A.x*B.x + A.y*B.y;
    return(Result);
}

/**
 * V3 Operations
 **/

union v3
{
    struct
    {
        f32 x, y, z;
    };
    struct
    {
        f32 r, g, b;
    };
    struct
    {
        v2 xy;
        f32 _Reserved0;
    };
    f32 E[3];
};

inline v3
V3(f32 A, f32 B, f32 C)
{
    v3 Result = {};
    Result.x = A;
    Result.y = B;
    Result.z = C;
    return(Result);
}

inline v3
operator +(v3 A, v3 B)
{
    v3 Result = {};
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    return(Result);
}

inline v3&
operator +=(v3 &A, v3 B)
{
    A.x = A.x + B.x;
    A.y = A.y + B.y;
    A.z = A.z + B.z;
    return(A);
}

inline v3
operator -(v3 A, v3 B)
{
    v3 Result = {};
    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    return(Result);
}

inline v3&
operator -(v3 &A, v3 B)
{
    A.x = A.x - B.x;
    A.y = A.y - B.y;
    A.z = A.z - B.z;
    return(A);
}

inline v3
operator *(v3 A, f32 B)
{
    v3 Result = {};
    Result.x = A.x*B;
    Result.y = A.y*B;
    Result.z = A.z*B;
    return(Result);
}

inline v3
operator *(f32 A, v3 B)
{
    v3 Result = B*A;
    return(Result);
}

inline v3
Hadamard(v3 A, v3 B)
{
    v3 Result = {};
    Result.x = A.x*B.x;
    Result.y = A.y*B.y;
    Result.z = A.z*B.z;
    return(Result);
}

inline f32
Inner(v3 A, v3 B)
{
    f32 Result = A.x*B.x + A.y*B.y + A.z*B.z;
    return(Result);
}

/**
 * V4 Operations
 **/

union v4
{
    struct
    {
        f32 x, y, z, w;
    };
    struct
    {
        f32 r, g, b, a;
    };
    struct
    {
        v3 xyz;
        f32 _Reserved0;
    };
    struct
    {
        v3 rgb;
        f32 _Reserved1;
    };
    f32 E[4];
};

inline v4
V4(f32 A, f32 B, f32 C, f32 D)
{
    v4 Result = {};
    Result.x = A;
    Result.y = B;
    Result.z = C;
    Result.w = D;
    return(Result);
}

inline v4
V4(v3 XYZ, f32 W)
{
    v4 Result = {};
    Result.x = XYZ.x;
    Result.y = XYZ.y;
    Result.z = XYZ.z;
    Result.w = W;
    return(Result);
}

inline v4
operator +(v4 A, v4 B)
{
    v4 Result = {};
    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    Result.w = A.w + B.w;
    return(Result);
}

inline v4
operator *(v4 A, f32 B)
{
    v4 Result = {};
    Result.x = A.x*B;
    Result.y = A.y*B;
    Result.z = A.z*B;
    Result.w = A.w*B;
    return(Result);
}

inline v4
operator *(f32 A, v4 B)
{
    v4 Result = B*A;
    return(Result);
}

/**
 * Rectangle2 Operation
 **/
struct rect2
{
    v2 Min;
    v2 Max;
};

inline rect2
Rect2(v2 Min, v2 Max)
{
    rect2 Result = {};
    Result.Min = Min;
    Result.Max = Max;
    return(Result);
}

inline rect2
AddRadiusTo(rect2 Rect, v2 Radius)
{
    rect2 Result = Rect;
    Result.Min -= Radius;
    Result.Max += Radius;
    return(Result);
}

inline b32
IsInRectangle(rect2 Rect, v2 Point)
{
    b32 Result = ( (Point.x >= Rect.Min.x) &&
                   (Point.x < Rect.Max.x) &&
                   (Point.y >= Rect.Min.y) &&
                   (Point.y < Rect.Max.y) );
    return(Result);
}

#define MINESWEEPER_MATH_H
#endif
