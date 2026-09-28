# Aura model assets

`aura.3ds` and `aura_map.jpg` originate from NASA Ames Research Center's
[Aura (A) model](https://science.nasa.gov/3d-resources/aura-a/).
The original 3DS assets are available in the
[NASA 3D Resources historical tree](https://github.com/nasa/NASA-3D-Resources/tree/05374ee6e3b254cf869d3c19fb2f9bddb05a20c0/3D%20Models/Aura%20(A)).

The original model contains an optional reflection map for `gold_foil` named
`GFOIL1.JPG`, which is absent from both this bundle and that source directory.
This copy removes only that 31-byte MAT_REFLMAP (0xA220) chunk and updates the
three enclosing chunk lengths. Geometry, UV coordinates, material properties,
nine diffuse-map references and the supplied image are unchanged. Both GMAT's
native loader and OpenSceneGraph produce the same loaded model as before; the
unresolvable optional reflection map no longer causes a missing-image warning.
The diffuse atlas must not be supplied as an invented reflection map.

Original Git blob IDs (before this repair):

- `aura.3ds`: `1a5eeff4fc9f485604620497561a5b1228fb9512`
- `aura_map.jpg`: `67a07430a4eabd1b041b4ee39654ef5c259192b8`
