from pathlib import Path
import math, wave, struct
root=Path(__file__).resolve().parents[1]/'assets'
words={
'noun': 'garden friend cloud book kitten window river flower morning music story paper light home forest ocean meadow star moon tea blanket breeze apple bread door tree leaf rain sky path song desk pencil school chair world heart bird paw sun mountain field smile',
'verb': 'write learn smile walk read draw listen share think play rest grow open find make give take bring help look watch follow begin finish enjoy create hold keep move dream jump dance sing build explore sit stand sleep wake work try care laugh',
'helper': 'is are was were be been being have has had do does did can could will would shall should may might must',
'adjective': 'gentle bright happy quiet sunny soft kind little warm cool fresh calm lovely curious brave tiny sweet clear simple cosy smooth golden green blue violet white quick slow young friendly patient peaceful pretty round small sleepy',
'pronoun': 'we they she you he it i us them her him our your their its my',
'adverb': 'slowly often always softly kindly gently soon today now here there again together quietly quickly really very never sometimes happily',
'preposition': 'under near with above below beside between across around through into over from for to on in at by behind before after without',
'other': 'the a an and but because or so if while when as then yet also each every some any both many more most this that these those yes no'
}
(root/'vocabulary.tsv').write_text('# category\tword — original bundled English vocabulary\n'+''.join(f'{c}\t{w}\n' for c,ws in words.items() for w in ws.split()))
expressions=['welcome','encouragement','celebration','smile','wink','curious']
for expression in expressions:
    open_eyes='<ellipse cx="62" cy="64" rx="4.5" ry="6" fill="#493257"/><ellipse cx="98" cy="64" rx="4.5" ry="6" fill="#493257"/><circle cx="63" cy="62" r="1.3" fill="white"/><circle cx="99" cy="62" r="1.3" fill="white"/>'
    eyes=open_eyes if expression in ['welcome','curious'] else '<path d="M56 65q6-9 12 0 M92 65q6-9 12 0" fill="none" stroke="#493257" stroke-width="3.5" stroke-linecap="round"/>'
    if expression=='wink':
        eyes='<ellipse cx="62" cy="64" rx="4.5" ry="6" fill="#493257"/><circle cx="63" cy="62" r="1.3" fill="white"/><path d="M92 65q6-9 12 0" fill="none" stroke="#493257" stroke-width="3.5" stroke-linecap="round"/>'
    mouth='<path d="M80 80v4m0 0q-6 7-11 0m11 0q6 7 11 0" fill="none" stroke="#9E7B9E" stroke-width="2" stroke-linecap="round"/>'
    if expression in ['smile','wink']:
        mouth='<path d="M70 83Q80 89 90 83Q88 97 80 97Q72 97 70 83Z" fill="#E9A8C3" stroke="#9E7B9E" stroke-width="1.8"/><path d="M76 94Q80 90 84 94" fill="none" stroke="#F7D2DF" stroke-width="2.5" stroke-linecap="round"/>'
    elif expression=='curious':
        mouth='<ellipse cx="80" cy="88" rx="3.5" ry="4.5" fill="#E9A8C3" stroke="#9E7B9E" stroke-width="1.8"/>'
    paws = [(48,109,0),(121,83,-18)] if expression in ['encouragement','wink'] else [(31,79,18),(129,79,-18)] if expression=='celebration' else [(51,110,0),(109,110,0)]
    pawsvg=''.join(f'<g transform="translate({x} {y}) rotate({r})"><ellipse cx="0" cy="0" rx="14" ry="18" fill="#fffdfd" stroke="#BAABC9" stroke-width="2.5"/><ellipse cx="0" cy="4" rx="5" ry="4" fill="#F3C0D5"/><circle cx="-6" cy="-3" r="2.4" fill="#F3C0D5"/><circle cx="0" cy="-6" r="2.4" fill="#F3C0D5"/><circle cx="6" cy="-3" r="2.4" fill="#F3C0D5"/></g>' for x,y,r in paws)
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 160 135"><ellipse cx="80" cy="121" rx="45" ry="6" fill="#B8A0D5" opacity=".16"/>{pawsvg}<path d="M38 44L37 15Q38 10 43 14L62 29Q80 22 98 29L117 14Q122 10 123 16L122 44Q137 58 128 80Q117 103 80 105Q43 103 32 80Q23 58 38 44Z" fill="#fffdfd" stroke="#BAABC9" stroke-width="2.5" stroke-linejoin="round"/><path d="M43 23L44 42L57 33Z M117 23L103 33L116 42Z" fill="#F5C8D8"/>{eyes}<ellipse cx="48" cy="78" rx="10" ry="5" fill="#F3C0D5" opacity=".7"/><ellipse cx="112" cy="78" rx="10" ry="5" fill="#F3C0D5" opacity=".7"/><path d="M76 76Q80 73 84 76L80 80Z" fill="#D88AAB"/>{mouth}<path d="M111 30Q92 14 96 35Q96 46 111 38Q127 51 127 32Q125 18 111 30Z" fill="#A477D5" stroke="#8553C2" stroke-width="1.5"/><circle cx="111" cy="34" r="4.5" fill="#CBA9EE"/></svg>'''
    (root/f'kitten-{expression}.svg').write_text(svg)
(root/'paw.svg').write_text('''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 40 40"><g fill="#A477D5"><ellipse cx="20" cy="27" rx="12" ry="9"/><ellipse cx="7" cy="17" rx="4" ry="6" transform="rotate(-25 7 17)"/><ellipse cx="15" cy="9" rx="4" ry="6"/><ellipse cx="25" cy="9" rx="4" ry="6"/><ellipse cx="33" cy="17" rx="4" ry="6" transform="rotate(25 33 17)"/></g></svg>''')
(root/'restart.svg').write_text('''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="M5 8a8 8 0 1 1-1 7M5 3v6h6" fill="none" stroke="#A477D5" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>''')
(root/'chevron.svg').write_text('''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="m6 9 6 6 6-6" fill="none" stroke="#A477D5" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/></svg>''')
for name,seconds in [('typing',.045),('word',.12),('backspace',.055),('encouragement',.32),('celebration',.45),('click',.035)]:
    sr=22050
    with wave.open(str(root/f'{name}.wav'),'wb') as wav:
        wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(sr)
        samples=[]
        for i in range(int(sr*seconds)):
            t=i/sr
            if name=='click': value=(.16*math.sin(2*math.pi*1500*t)+.08*math.sin(2*math.pi*2300*t))*math.exp(-100*t)*min(1,t/.001)
            elif name=='typing': value=.11*math.sin(2*math.pi*730*t)*math.exp(-90*t)*min(1,t/.004)
            elif name=='backspace': value=.08*math.sin(2*math.pi*(460*t-900*t*t))*math.exp(-70*t)*min(1,t/.004)
            elif name=='word': value=.09*math.sin(2*math.pi*880*t)*math.exp(-32*t)*min(1,t/.006)
            else:
                value=0
                notes=[(0,659.25),(.09,880)] if name=='encouragement' else [(0,523.25),(.10,659.25),(.20,783.99)]
                for start,f in notes:
                    s=t-start
                    if s>=0: value+=.07*math.sin(2*math.pi*f*s)*math.exp(-10*s)*min(1,s/.01)
            # Fade the tail to silence, then normalize the deliberately soft
            # source oscillators. Quiet source * game volume * speaker volume
            # previously made these cues almost inaudible.
            value*=min(1,max(0,(int(sr*seconds)-1-i)/(sr*.006)))
            samples.append(value)
        peak=max(abs(s) for s in samples)
        rms=math.sqrt(sum(s*s for s in samples)/len(samples))
        gain=min(.55/peak,.16/rms)
        wav.writeframes(b''.join(struct.pack('<h',round(s*gain*32767)) for s in samples))
files=['vocabulary.tsv']+[f'kitten-{expression}.svg' for expression in expressions]+['paw.svg','restart.svg','chevron.svg','typing.wav','word.wav','backspace.wav','encouragement.wav','celebration.wav','click.wav']
(root/'resources.qrc').write_text('<RCC><qresource prefix="/assets">'+''.join(f'<file>{f}</file>' for f in files)+'</qresource></RCC>')
