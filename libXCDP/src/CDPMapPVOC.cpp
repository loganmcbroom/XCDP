#include "PVOC.h"

#include "RealFunc.h"

namespace xcdp {

//========================================================================
// Blur
//========================================================================

PVOC PVOC::blur_blur( RealFunc blur ) const
	{ 
	auto frameFixBlur = [&blur, this]( double t ){ return frameToTime( blur( t ) ); };
	return desample( frameFixBlur, frameFixBlur, linearInterpolator ); 
	}

PVOC PVOC::blur_chorus( RealFunc fspread ) const
	{
	return perturb( 0, fspread, identityPerturber, normalDistPerturber );
	}

//========================================================================
// Combine
//========================================================================

PVOC PVOC::combine_cross( const PVOC & ampSource, RealFunc amount ) const
	{
	return replaceAmplitudes( ampSource, amount );
	}

}