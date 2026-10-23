/* Ordinary records with bitfields. Inputs and outputs each contain count
   records; calls may observe state and pointer aliases. */
typedef struct Packet {
    unsigned tag:5;
    signed int drift:7;
    unsigned mask:12;
    unsigned spare:8;
    unsigned weight, salt;
} Packet;
extern unsigned observe_packet(const Packet *, unsigned);

static unsigned rotate_word(unsigned value, unsigned shift)
{
    shift=(shift&15)+1;
    return (value<<shift)|(value>>(32-shift));
}

static unsigned sample_packet(const Packet *p, unsigned key)
{
    return rotate_word(p->weight+key,p->tag)+p->salt+
        (unsigned)(p->drift*17)+(p->mask^key);
}

static Packet change_packet(Packet value, unsigned mix)
{
    value.tag=(value.tag+mix)&31;
    value.drift=(int)((mix>>3)&63)-31;
    value.mask^=mix&4095;
    value.weight+=mix;
    value.salt^=rotate_word(mix,value.spare);
    return value;
}

static unsigned update_packet(Packet *p, unsigned mix)
{
    Packet value=change_packet(*p,mix);
    *p=value;
    return sample_packet(p,mix);
}

unsigned inline_records(Packet *out, const Packet *in, unsigned count,
    unsigned seed)
{
    Packet state;
    unsigned i, visits=0, left, right;
    for(i=0;i<count;++i) {
        state=in[i];
        left=update_packet(&state,seed+visits++);
        right=sample_packet(&state,rotate_word(left,seed));
        switch((state.tag+seed)&15) {
        case 0: seed+=left; break;
        case 1: seed^=right; break;
        case 2: seed+=left-right; break;
        case 3: seed^=left+right; break;
        case 4: seed+=rotate_word(left,3); break;
        case 5: seed^=rotate_word(right,5); break;
        case 6: seed+=state.mask; break;
        case 7: seed^=(unsigned)state.drift; break;
        case 8: seed+=state.weight; break;
        case 9: seed^=state.salt; break;
        case 10: seed+=left^right; break;
        case 11: seed^=left-right; break;
        default: seed+=sample_packet(&state,seed); break;
        }
        out[i]=change_packet(state,seed+visits++);
        seed+=observe_packet(out+i,seed^visits);
    }
    return seed+visits;
}
