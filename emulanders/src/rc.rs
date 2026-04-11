pub const RESULT_MODULE: u32 = 352;

result_define_group!(RESULT_MODULE => {
    VirtualSkylanderFlagNotFound: 1,
    VirtualSkylanderJsonNotFound: 2,
    InvalidJsonSerialization: 3,
    InvalidJsonDeserialization: 4,
    InvalidLoadedVirtualSkylander: 5,
    VirtualSkylanderAreasJsonNotFound: 6,
    InvalidActiveVirtualSkylander: 7,
    InvalidVirtualSkylanderAccessId: 8,
    InvalidDeprecatedVirtualSkylander: 9
});