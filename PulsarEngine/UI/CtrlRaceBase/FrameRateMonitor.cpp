namespace Pulsar {
namespace UI { 

static float framerate = 0.0;

asmFunc WriteFramerateToEVA() {
    ASM(
    nofralloc;
    stfs      f0, 0x24(r31);
    lis       r12, framerate@ha;
    ori       r12, r12, framerate@l;
    stfs      f0, 0x0(r12);
    blr;
    )
}
kmCall(0x8021a0d8, WriteFramerateToEVA);


void CtrlRaceSpeedo::Create(Page& page, u32 index, u32 count) {
    CtrlRaceSpeedo* som = new(CtrlRaceSpeedo);
    page.AddControl(index + 1, *som, 0);
    char variant[0x20];
    snprintf(variant, 0x20, "Speedo_0_0");
    som->Load(variant, i);
    }
}
static CustomCtrlBuilder CtrlRaceSpeedo::Create;

}
}