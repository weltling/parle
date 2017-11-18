

#ifndef PARLE_OSTREAM_HPP
#define PARLE_OSTREAM_HPP

#if PARLE_U32
#include <locale>
#include <codecvt>

namespace parle
{
#ifndef ZTS
	static std::wstring_convert<std::codecvt_utf8<parle::char_type>, parle::char_type> cvt;
#else
	static thread_local std::wstring_convert<std::codecvt_utf8<parle::char_type>, parle::char_type> cvt;
#endif
}

#define PARLE_CVT_U32(sptr) parle::cvt.from_bytes(sptr).c_str()
#define PARLE_SCVT_U32(s) parle::cvt.from_bytes(s)
#if defined(_MSC_VER)
#define PARLE_PRE_U32(ca) PARLE_SCVT_U32(ca)
#else
#define PARLE_PRE_U32(ca) U ## ca
#endif
#define PARLE_CVT_U8(sptr) parle::cvt.to_bytes(sptr).c_str()
#define PARLE_SCVT_U8(s) parle::cvt.to_bytes(s)
#else
#define PARLE_CVT_U32(sptr) sptr
#define PARLE_SCVT_U32(s) s
#define PARLE_PRE_U32(ca) ca
#define PARLE_CVT_U8(sptr) sptr
#define PARLE_SCVT_U8(s) s
#endif

#endif /* PARLE_PHP_OSTREAM_CPP */
 
