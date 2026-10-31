"""Reproduce the 0.3 factory bank: distinct chains, not variants of one riser."""
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
def m(a, b=None, begin=0, end=1, curve=1, log=0):
    return [a, a if b is None else b, begin, end, curve, log]
def slot(kind, values, wet=(0, 1), start=0, curve=.75):
    maps = [m(0) for _ in range(8)]
    for i, value in enumerate(values):
        maps[i] = value if isinstance(value, list) else m(value)
    maps[7] = m(*wet, begin=start, curve=curve)
    return dict(type=kind, enabled=True, mappings=maps)
def hp(hz=2400, q=.8):
    return slot(1, [m(25,hz,log=1), q, 1], (1,1))
def lp(hz=450, q=.8):
    return slot(2, [m(19000,hz,log=1), q, 1], (1,1))
def delay(beats=.75, feedback=.65, wet=.65, tape=False, ping=True, gain=0):
    return slot(6 if tape else 5, [beats,m(.15,feedback),m(12000,3200,log=1),.1,int(ping),1,gain], (0,wet))
def room(wet=.75, size=.93, dark=.35, predelay=.025):
    return slot(7,[m(.35,size),dark,1,predelay,-6],(0,wet),curve=.7)
def crush(bits=4, rate=1800):
    return slot(13,[m(16,bits),m(48000,rate,log=1),m(19000,11000,log=1)],(0,1),curve=.55)
def drive(db=15, wet=.6):
    return slot(14,[m(0,db)],(0,wet))
def chop(speed=4, depth=1):
    return slot(9,[speed,m(0,depth,curve=.65),m(1,4)],(1,1))
def pan(speed=.5, depth=1):
    return slot(10,[speed,m(0,depth)],(1,1))
def loop(reverse=False, accelerate=True, division=0, start=.18):
    return slot(8,[m(0,5,begin=start) if accelerate else division,int(reverse),start,.5],(0,1),start,curve=.55)
def phase(speed=.25, feedback=.65, wet=.65):
    return slot(12,[speed,m(.2,1),m(0,feedback),.5],(0,wet),curve=.6)
def flange(speed=.125, feedback=.7, negative=False, wet=.5):
    return slot(18,[speed,m(.4,5),m(0,feedback),.3,int(negative)],(0,wet),curve=.55)
def chorus(wet=.5):
    return slot(11,[m(.15,.6),m(.3,1)],(0,wet))
def siren(note=57, interval=12, speed=1, level=.13):
    return slot(15,[note,interval,m(.5,9),speed,0,m(0,level,begin=.15)],(1,1))
def noise(start=300, end=13000, level=.14, pink=1):
    return slot(16,[m(start,end,log=1),m(0,level,begin=.08),pink],(1,1))
def tone(low=0, mid=0, high=0):
    return slot(4,[m(0,low),m(0,mid),m(0,high)],(1,1))
def level(db):
    return slot(17,[m(0,db,curve=.65),1,0],(1,1))

bank=[]
def add(name, category, description, chain):
    assert len(chain)<=6
    bank.append(dict(version=1,name=name,category=category,description=description,
                     mix=1,output=0,slots=chain+[slot(0,[],(0,0)) for _ in range(6-len(chain))]))

add('Disco Lift','Flanger','Wide jet sweep with resonant positive-feedback flanging, a bright shelf and a small room; no high-pass or echo.',[flange(),tone(-2,0,3),room(.2,.5)])
add('Jackin Pressure','Gate','Hard eighth-note chopping into crunchy drive. A rhythmic dry-bus transformation without filter or delay.',[chop(2),drive(18,.8)])
add('Hands Up Wash','Filter + reverb','A resonant high-pass sweep dissolves into an almost fully wet, bright large room; no echo.',[hp(3500,1.1),room(.94,.98,.15,.04)])
add('Late Night Echo','Delay','An exposed dotted-eighth ping-pong tape throw, with dark feedback and no filter sweep or room.',[delay(.75,.76,.82,True)])
add('Snare Ladder','Stutter','Captured forward slices accelerate from a quarter note to a 1/128-note buzz, with increasing grit.',[loop(),drive(12,.4)])
add('Digital Meltdown','Crusher','Full-band four-bit, 1.4 kHz sample-and-hold destruction. No high-pass, delay or reverb hides the aliasing.',[crush(4,1400)])
add('Warehouse Siren','Siren + flanger','A rising octave siren passes through resonant flanging and quarter-note repeats.',[siren(57,12,1,.13),flange(.25,.55),delay(.5,.52,.42)])
add('White Heat','Noise + reverb','A bright white-noise ascent spreads into a huge low-damping room; no high-pass or discrete echo.',[noise(900,16000,.18,0),room(.8,.97,.12)])
add('Vinyl Tunnel','Phaser','A deep stereo four-stage phaser with feedback, followed by a slowly narrowing band-pass corridor.',[phase(.125,.72,.7),slot(3,[m(1200,650,log=1),m(.5,2),0],(0,.65))])
add('Reverse Tease','Reverse loop','A held reversed eighth-note fragment takes over early, then blooms into a short dark room.',[loop(True,False,1,.12),room(.3,.55,.7)])
add('Pump It Up','Gate + pan','Sixteenth-note hard gating alternates with fast stereo movement. No tonal sweep or time effect.',[chop(4),pan(1,1)])
add('The Big Finish','Multi effect','Six stages: low-pass closure, five-bit crush, ping-pong delay, huge reverb, accelerating stutter and jet flanging.',[lp(800),crush(5,3200),delay(.5,.62,.55),room(.65,.92),loop(False,True,0,.5),flange(.5,.5)])
add('Velvet Balcony','Low-pass','A smooth, deep low-pass descent to 220 Hz. Pure filter movement without wash, echo or modulation.',[lp(220,.85)])
add('Vocal Comet','Delay + reverb','A full half-note stereo throw rings into a bright large room; the dry low end is not swept away.',[delay(2,.7,.75,False),room(.5,.9,.2,.06)])
add('Rooftop Shimmer','Chorus','Deep moving chorus, air shelf and restrained ambience. No high-pass, stutter or repeating echo.',[chorus(.7),tone(0,-2,5),room(.18,.6,.2)])
add('Bassline Orbit','Negative flanger','Negative-polarity comb sweeps orbit the stereo field while a gentle low-pass darkens the top.',[flange(.0625,-.7,True,.55),lp(2200,.65)])
add('Percussion Staircase','Held loop + gate','A whole quarter-note loop locks in early and stays at the same length while triplet gating intensifies.',[loop(False,False,0,.1),chop(3,.95)])
add('Neon Exit','Crusher + delay','Eight-bit telephone-rate crunch closes through a resonant low-pass into saturated short mono tape repeats.',[crush(8,6500),lp(650,1.2),drive(22,.65),delay(.375,.62,.5,True,False)])
add('Pink Champagne','Noise + pan','The input recedes beneath a descending low pink-noise swell, slow stereo movement and a small dark room.',[level(-16),noise(5000,500,.2,1),pan(.25,.9),room(.3,.4,.85)])
add('Dub Elevator','Dub delay + phaser','Bass shelf removal feeds long dotted-quarter tape feedback, then a slow resonant phase sweep with a compensated wet return.',[tone(-15,3,-3),delay(1.5,.82,.8,True,gain=-9),phase(.0625,.5,.5)])
add('Midnight Pendulum','Pan + delay','Full-width triplet panning with triplet-quarter clean repeats and fast phase movement.',[pan(1.5),delay(2/3,.55,.65),phase(.75,.3,.5)])
add('Starlight Call','Falling siren','A descending C5 siren with fast vibrato floats above long dark reverb; no discrete echo.',[siren(72,-12,3,.1),room(.75,.92,.7,.08)])
add('Filter Confetti','High-pass','Pure resonant 24 dB/octave high-pass rise to 6 kHz. No echo, modulation, bit reduction or reverb.',[hp(6000,1.8)])
add('Last Train Home','Low-pass + reverb','A deep low-pass descent smears into an almost fully wet dark hall, with a late held reverse sixteenth-note loop.',[lp(380,.7),room(.94,.98,.8,.015),loop(True,False,2,.7)])

assert len(bank)==24
(root/'Resources/factory.json').write_text(json.dumps(bank,indent=2)+'\n')
print(f'{len(bank)} factory configurations; {len({tuple(s["type"] for s in p["slots"]) for p in bank})} distinct ordered chains')
