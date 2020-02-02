# Written by Egg Boy Color
# Taken from a longer script found at: https://twitter.com/eggboycolor/status/812521855676518400

unique_frequencies = {}

for r in range(8):
    for s in range(15):
        f = int(524288.0 / (r if r else 0.5)) >> (s+1)
        if f > 64:
            unique_frequencies[f] = (s << 4) | r

freq = ['0xF7'] * 3
for k in sorted(unique_frequencies):
    print('        // r = ' + str(unique_frequencies[k] & 0x7) + ', s = ' + str(unique_frequencies[k] >> 4) + ', f = ' + str(k) + 'hz')
    freq.append(hex(unique_frequencies[k]))
for k in sorted(unique_frequencies):
    freq.append(hex(unique_frequencies[k] | 0x8))
freq += ['0xFF'] * (128 - len(freq))

