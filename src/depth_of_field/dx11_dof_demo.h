#pragma once

#include "pch.h"
#include "prerequisites.h"

struct cb_per_obj
{
    XMMATRIX mat_wvp;
    XMMATRIX mat_inv_proj; 
};

struct cb_per_frame
{
    XMVECTOR vec_depth;
}; 