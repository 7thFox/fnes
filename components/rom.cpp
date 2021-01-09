#include "rom.h"

namespace components
{
Rom::Rom(uint8_t *image, uint16_t *addr, uint8_t *data)
{
    this->image = image;
    this->address = addr;
    this->data = data;
}

Rom::~Rom() {}

void Rom::set_clk()
{
    // uint8_t newClk = sig & 1;
    // if (newClk == 1 && this->clk == 0) // Rising-edge
    // {
    if (*this->address >= 0x4020)
    {
        (*this->data) = this->image[(*this->address) - 0x4020];
    }
    // }
    // this->clk = newClk;
}

uint8_t Rom::debug_get_at_addr(uint16_t addr, char *strbytes, char *strdis)
{
    if (addr >= 0x4020)
    {
        auto op = this->image[addr - 0x4020];
        uint8_t lo = 0x00;
        if (addr+1 <= 0xFFFF){
            lo = this->image[addr - 0x4020 + 1];
        }
        uint8_t hi = 0x00;
        if (addr+2 <= 0xFFFF){
            hi = (this->image[addr - 0x4020 + 2]);
        }
        auto param16 = lo | (hi << 8);
        auto meta = Cpu6502::metadata[op];
        uint8_t bytes = meta.nbytes();
        switch (meta.addr_mode)
        {
            case AddressingMode::A:
                sprintf(strdis, "%s A", meta.mnemonic.c_str());
                break;
            case AddressingMode::abs:
                sprintf(strdis, "%s $%04x  (%d)", meta.mnemonic.c_str(), param16, param16);
                break;
            case AddressingMode::absX:
                sprintf(strdis, "%s $%04x,X  (%d)", meta.mnemonic.c_str(), param16, param16);
                break;
            case AddressingMode::absY:
                sprintf(strdis, "%s $%04x,Y  (%d)", meta.mnemonic.c_str(), param16, param16);
                break;
            case AddressingMode::imm:
                sprintf(strdis, "%s #$%02x  (%d)", meta.mnemonic.c_str(), lo, lo);
                break;
            case AddressingMode::impl:
                sprintf(strdis, "%s", meta.mnemonic.c_str());
                break;
            case AddressingMode::ind:
                sprintf(strdis, "%s ($%04x)  (%d)", meta.mnemonic.c_str(), param16, param16);
                break;
            case AddressingMode::Xind:
                sprintf(strdis, "%s ($%02x,X)  (%d)", meta.mnemonic.c_str(), lo, lo);
                break;
            case AddressingMode::indY:
                sprintf(strdis, "%s ($%02x),Y  (%d)", meta.mnemonic.c_str(), lo, lo);
                break;
            case AddressingMode::rel:
                sprintf(strdis, "%s $%02x  (%d)", meta.mnemonic.c_str(), lo, lo);
                break;
            case AddressingMode::zpg:
                sprintf(strdis, "%s $%02x  (%d)", meta.mnemonic.c_str(), lo, lo);
                break;
            case AddressingMode::zpgX:
                sprintf(strdis, "%s $%02x,X  (%d)", meta.mnemonic.c_str(), lo, lo);
                break;
            case AddressingMode::zpgY:
                sprintf(strdis, "%s $%02x,Y  (%d)", meta.mnemonic.c_str(), lo, lo);
                break;
        }
    
        switch (bytes)
        {
        case 3:
            sprintf(strbytes, "%02x %02x %02x ", op, lo, hi);
            break;
        case 2:
            sprintf(strbytes, "%02x %02x    ", op, lo);
            break;
        case 1:
            sprintf(strbytes, "%02x       ", op);
            break;

        default:
            strbytes[0] = '\0';
            break;
        }

        return bytes;
    }
    strdis[0] = '\0';
    strbytes[0] = '\0';
    return 0;
}
}