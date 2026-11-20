#pragma once
struct CvarAddress{const char* name;unsigned rva;};
static const CvarAddress kCvars[]={
 {"r_singleArea",0x7104b4},
 {"r_occlusionCullMasterEnable",0x710620},
 {"r_occlusionCullPortals",0x710688},
 {"r_occlusionCullDynamicModels",0x7106bc},
 {"r_useEntityCulling",0x710210},
 {"r_useLightCulling",0x7101a8},

 {"r_portalsDistanceCull",0x7105ec},
};


