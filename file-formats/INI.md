# INI — Track Configuration Files

> Per-track settings for audio, physics, camera, and metadata.

Track directories contain multiple `.ini` files that configure various aspects of each track.

## info.ini

Track metadata and race configuration:

```ini
[Track]
ID=1
Name=Boothill
BaseElevation=1200        ; feet above sea level
Temperature=75             ; track temperature (affects tire physics?)

[Segments]
StartFinish=0             ; start/finish line segment index
GridSpread=5               ; pole position spread
NumLaps=3                  ; default lap count

[SpeedTraps]
Count=2
0_Slice=127               ; speed trap 0: segment index
0_Lane=2                  ; speed trap 0: lane
1_Slice=381
1_Lane=2
```

## audio.ini / audioW.ini / audioN.ini

Audio bank assignments per track:

```ini
[Audio]
Bank=track.bnk
MusicVolume=0.85
SFXVolume=0.90
```

Variant suffixes (`W`=west?, `N`=north?) may correspond to different audio zones or camera positions.

## boom.ini

Crash and explosion audio configuration:

```ini
[Boom]
Bank=boom.bnk
Volume=0.80
```

## pavement.ini

Surface physics parameters per track surface:

```ini
[Surface]
Friction=0.85              ; grip coefficient
RollingResistance=0.02     ; rolling drag
```

## TrCam.ini / TrCamNW.ini

Camera waypoints for replay and spectator system:

```ini
[Camera]
Count=24
0_Pos=(1234.5, 10.2, -456.7)
0_Target=(1240.0, 5.0, -450.0)
1_Pos=(1245.3, 11.0, -445.2)
1_Target=(1250.0, 5.0, -440.0)
; ... etc
```

## trmap.txt

Minimap text labels:

```
START/FINISH
TURN 1
CHICANE
SPEED TRAP
PIT ENTRY
```

## Status

✅ **Documented** — structure is human-readable INI format.
