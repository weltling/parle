/*
 * Copyright (c) 2022 Anatol Belski
 * All rights reserved.
 *
 * Author: Anatol Belski <ab@php.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* $Id$ */


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
 
