#include "motor_display.hpp"
#include <pvtui/pvtui.hpp>
#include <ftxui/component/component.hpp>

SmallMotorDisplay::SmallMotorDisplay(pvtui::PVGroup& pvgroup, const std::string& record)
    : record_name(record),
    desc(pvgroup, record + ".DESC", pvtui::PVPutType::String),
    val(pvgroup, record + ".VAL", pvtui::PVPutType::Double),
    twr(pvgroup, record + ".TWR", " < "),
    twv(pvgroup, record + ".TWV", pvtui::PVPutType::Double),
    twf(pvgroup, record + ".TWF", " > "),
    rbv(pvgroup, record + ".RBV"),
    dmov(pvgroup, record + ".DMOV"),
    lls(pvgroup, record + ".LLS"),
    hls(pvgroup, record + ".HLS"),
    lvio(pvgroup, record + ".LVIO"),
    egu(pvgroup, record + ".EGU"),
    able(pvgroup, record + "_able"),
    use_set(pvgroup, record + ".SET", pvtui::ChoiceStyle::Horizontal),
    stop(pvgroup, record + ".STOP", " STOP ")
{

    auto container = ftxui::Container::Vertical({
        desc.component(),
        val.component(),
        twr.component(),
        twv.component(),
        twf.component(),
        use_set.component(),
        stop.component(),
        egu.component(),
    }) | ftxui::Renderer([this](ftxui::Element){
        return this->render();
    });

    Add(container);
}

ftxui::Element SmallMotorDisplay::render() {
    using namespace ftxui;
    using namespace pvtui;

    auto val_bg = able.value().index == 1 ? bgcolor(ftxui::Color::DarkRed) | color(ftxui::Color::Black) :
    lvio.value() == 1 ? bgcolor(Color::Yellow2) :
    EPICSColor::edit(val);

    return vbox({
        desc.component()->Render()
            | EPICSColor::custom(desc, color(Color::Black) | bgcolor(Color::RGB(210,210,210)))
            | size(WIDTH, EQUAL, 20)
            | bold
            | center,
        hbox({
            filler() | size(WIDTH, EQUAL, egu.value().size()+1),
            text(lls.value() ? unicode::rectangle(1) : "  ")
                | center
                | color(Color::Red),
            text(rbv.value())
                | center
                | size(WIDTH, EQUAL, 10)
                | (use_set.value().index==0 ? EPICSColor::readback(use_set) : color(Color::Yellow2))
                | (dmov.value() == 0 ? borderHeavy | color(Color::Green) : borderEmpty),
            text(hls.value() ? unicode::rectangle(1) : "  ")
                | center
                | color(Color::Red),
            text(egu.value())
                | center
                | EPICSColor::readback(egu)
        }) | center,

        hbox({
            val.component()->Render() | val_bg | size(WIDTH, EQUAL, 10)
        }) | center,
        separatorEmpty(),

        hbox({
            twr.component()->Render() | EPICSColor::custom(twr, color(Color::Black)),
            separatorEmpty(),
            twv.component()->Render() | size(WIDTH, EQUAL, 10) | EPICSColor::edit(twv),
            separatorEmpty(),
            twf.component()->Render() | EPICSColor::custom(twf, color(Color::Black)),
        }) | center,
        separatorEmpty(),
        hbox({
            use_set.component()->Render() | EPICSColor::edit(use_set),
            separatorEmpty(),
            separatorEmpty(),
            stop.component()->Render() | EPICSColor::custom(stop, color(Color::Red)) | bold,
        }) | center,
        separatorEmpty()
    }) | size(WIDTH, EQUAL, 24) | border | color(Color::Black) | center;

}


LargeMotorDisplay::LargeMotorDisplay(pvtui::PVGroup& pvgroup, const std::string& record)
    : record_name(record),
    desc(pvgroup, record + ".DESC", pvtui::PVPutType::String),
    val(pvgroup, record + ".VAL", pvtui::PVPutType::Double),
    twr(pvgroup, record + ".TWR", " < "),
    twv(pvgroup, record + ".TWV", pvtui::PVPutType::Double),
    twf(pvgroup, record + ".TWF", " > "),
    rbv(pvgroup, record + ".RBV"),
    dmov(pvgroup, record + ".DMOV"),
    lls(pvgroup, record + ".LLS"),
    hls(pvgroup, record + ".HLS"),
    lvio(pvgroup, record + ".LVIO"),
    egu(pvgroup, record + ".EGU"),
    able(pvgroup, record + "_able", pvtui::ChoiceStyle::Vertical),
    use_set(pvgroup, record + ".SET", pvtui::ChoiceStyle::Horizontal),
    stop(pvgroup, record + ".STOP", " STOP "),
    drbv(pvgroup, record + ".DRBV"),
    dval(pvgroup, record + ".DVAL", pvtui::PVPutType::Double),
    hlm(pvgroup, record + ".HLM", pvtui::PVPutType::Double),
    dhlm(pvgroup, record + ".DHLM", pvtui::PVPutType::Double),
    llm(pvgroup, record + ".LLM", pvtui::PVPutType::Double),
    dllm(pvgroup, record + ".DLLM", pvtui::PVPutType::Double),
    spmg(pvgroup, record + ".SPMG", pvtui::ChoiceStyle::Vertical),
    vmax(pvgroup, record + ".VMAX", pvtui::PVPutType::Double),
    velo(pvgroup, record + ".VELO", pvtui::PVPutType::Double),
    vbas(pvgroup, record + ".VBAS", pvtui::PVPutType::Double),
    accl(pvgroup, record + ".ACCL", pvtui::PVPutType::Double),
    mres(pvgroup, record + ".MRES", pvtui::PVPutType::Double),
    eres(pvgroup, record + ".ERES", pvtui::PVPutType::Double),
    rres(pvgroup, record + ".RRES", pvtui::PVPutType::Double),
    rtry(pvgroup, record + ".RTRY", pvtui::PVPutType::Integer),
    off(pvgroup, record + ".OFF", pvtui::PVPutType::Double),
    prec(pvgroup, record + ".PREC", pvtui::PVPutType::Integer),
    rlv(pvgroup, record + ".RLV", pvtui::PVPutType::Double),
    rval(pvgroup, record + ".RVAL", pvtui::PVPutType::Double),
    ueip(pvgroup, record + ".UEIP", pvtui::ChoiceStyle::Horizontal),
    urip(pvgroup, record + ".URIP", pvtui::ChoiceStyle::Horizontal),
    dir(pvgroup, record + ".DIR", pvtui::ChoiceStyle::Horizontal),
    cnen(pvgroup, record + ".CNEN", pvtui::ChoiceStyle::Horizontal),
    foff(pvgroup, record + ".FOFF", pvtui::ChoiceStyle::Dropdown),
    rrbv(pvgroup, record + ".RRBV")
{

    auto container = ftxui::Container::Vertical({
        desc.component(),
        val.component(),
        twr.component(),
        twv.component(),
        twf.component(),
        use_set.component(),
        stop.component(),
        hlm.component(),
        llm.component(),
        rlv.component(),
        dhlm.component(),
        dval.component(),
        dllm.component(),
        rval.component(),
        able.component(),
        spmg.component(),
        vmax.component(),
        velo.component(),
        vbas.component(),
        accl.component(),
        foff.component(),
        off.component(),
        dir.component(),
        egu.component(),
        mres.component(),
        eres.component(),
        rres.component(),
        rtry.component(),
        ueip.component(),
        urip.component(),
        prec.component(),
        cnen.component(),
    }) | ftxui::Renderer([this](ftxui::Element){
        return this->render();
    });

    Add(container);
}

ftxui::Element LargeMotorDisplay::render() {
    using namespace ftxui;
    using namespace pvtui;

    auto val_bg = able.value().index == 1 ? bgcolor(ftxui::Color::DarkRed) | color(ftxui::Color::Black) :
        lvio.value() == 1 ? bgcolor(Color::Yellow2) :
        EPICSColor::edit(val);

    auto drive = vbox({
        hbox({
            vbox({
            separatorEmpty(),
            text("Hi Limit: "),
            text("Readback: "),
            text("Absolute: "),
            text("Lo Limit: "),
            text("Relative: "),
            }) | color(Color::Black),
            vbox({
            text("User") | center | color(Color::Black),
            hlm.component()->Render() | EPICSColor::edit(hlm),
            text(rbv.value())
                | (use_set.value().index==0 ? EPICSColor::readback(use_set) : color(Color::Yellow2))
                | center,
            val.component()->Render()
                | center
                | val_bg,
            llm.component()->Render() | EPICSColor::edit(llm),
            rlv.component()->Render() | EPICSColor::edit(rlv)
            }) | size(WIDTH, EQUAL, 10),
            separatorEmpty(),
            vbox({
            text("Dial") | center | color(Color::Black),
            dhlm.component()->Render() | EPICSColor::edit(dhlm),
            text(drbv.value())
                | (use_set.value().index==0 ? EPICSColor::readback(use_set) : color(Color::Yellow2))
                | center,
            dval.component()->Render()
                | center
                | val_bg,
            dllm.component()->Render() | EPICSColor::edit(dllm),
            filler() | size(WIDTH, EQUAL, 10),
            }) | size(WIDTH, EQUAL, 10),
            separatorEmpty(),
            vbox({
            text("Raw") | center | color(Color::Black),
            hls.value() == 1 ? text(unicode::rectangle(2)) | color(Color::Red) : text(""),
            text(rrbv.value()) | EPICSColor::readback(rrbv),
            rval.component()->Render() | size(WIDTH, EQUAL, 6) | EPICSColor::edit(rval),
            lls.value() == 1 ? text(unicode::rectangle(2)) | color(Color::Red) : text(""),
            }),
            separatorEmpty(),
            vbox({
            dmov.value()==0 ? text("Moving")
                | color(Color::Green)
                | bgcolor(Color::Black)
                | bold | italic : text(""),
            filler(),
            able.component()->Render() | size(WIDTH, EQUAL, 7) | EPICSColor::edit(able)
            }),
            separatorEmpty(),
            vbox({
            filler(),
            spmg.component()->Render() | size(WIDTH, EQUAL, 7) | EPICSColor::edit(spmg)
            }),
        }),
        hbox({
            text("Twk:") | color(Color::Black),
            twr.component()->Render() | EPICSColor::custom(twr, color(Color::Black)),
            separatorEmpty(),
            twv.component()->Render() | size(WIDTH, EQUAL, 10) | EPICSColor::edit(twv),
            separatorEmpty(),
            twf.component()->Render() | EPICSColor::custom(twf, color(Color::Black)),
        }),
    });

    auto dynamics = vbox({
        hbox({
            text(" Max Speed: ") | color(Color::Black),
            separatorEmpty(),
            vmax.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(vmax),
        }),

        hbox({
            text("     Speed: ") | color(Color::Black),
            separatorEmpty(),
            velo.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(velo),
        }),

        hbox({
            text(" Min Speed: ") | color(Color::Black),
            separatorEmpty(),
            vbas.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(vbas),
        }),

        hbox({
            text("     Accel: ") | color(Color::Black),
            separatorEmpty(),
            accl.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(accl),
        }),
    }) | size(WIDTH, EQUAL, 26);

    auto calibration = vbox({
        hbox({
            text("Off: ") | color(Color::Black),
            off.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(off),
            separatorEmpty(),
            foff.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(foff),
        }),
        hbox({
            text("Cal: ") | color(Color::Black),
            use_set.component()->Render() | size(WIDTH, EQUAL, 7) | EPICSColor::edit(use_set),
        }),
        hbox({
            text("Dir: ") | color(Color::Black),
            dir.component()->Render() | size(WIDTH, EQUAL, 7) | EPICSColor::edit(dir),
        }),
        hbox({
            text("EGU: ") | color(Color::Black),
            egu.component()->Render() | size(WIDTH, EQUAL, 7) | EPICSColor::edit(egu),
        }),
    }) | size(WIDTH, EQUAL, 26);

    auto res_left = vbox({
        hbox({
            text("   Motor res: ") | color(Color::Black),
            mres.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(mres),
        }),
        hbox({
            text(" Encoder res: ") | color(Color::Black),
            eres.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(eres),
        }),
        hbox({
            text("Readback res: ") | color(Color::Black),
            rres.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(rres),
        }),
        hbox({
            text("     Retries: ") | color(Color::Black),
            rtry.component()->Render() | size(WIDTH, GREATER_THAN, 7) | EPICSColor::edit(rtry),
        }),
    }) | size(WIDTH, EQUAL, 26) | color(Color::Black);

    auto res_right= vbox({
        hbox({
            text("   Precision: ") | color(Color::Black),
            prec.component()->Render() | size(WIDTH, EQUAL, 6) | EPICSColor::edit(prec),
        }),
        hbox({
            text(" Use encoder: ") | color(Color::Black),
            ueip.component()->Render() | size(WIDTH, EQUAL, 6) | EPICSColor::edit(ueip),
        }),
        hbox({
            text("Use readback: ") | color(Color::Black),
            urip.component()->Render() | size(WIDTH, EQUAL, 6) | EPICSColor::edit(urip),
        }),
        separatorEmpty(),
        hbox({
            text("Torque: ") | color(Color::Black),
            cnen.component()->Render() | size(WIDTH, EQUAL, 14) | EPICSColor::edit(cnen),
        }),
    }) | size(WIDTH, EQUAL, 26) | color(Color::Black);

    auto title = hbox({
        desc.component()->Render()
            | EPICSColor::custom(desc, color(Color::Black) | bgcolor(Color::RGB(210,210,210)))
            | size(WIDTH, EQUAL, 26),
        filler(),
        text("(" + record_name + ")")
            | color(Color::Black)
            | bold
            | xflex,
    });

    return vbox({
        title,
        separator() | color(Color::Black),
        drive,
        separator() | color(Color::Black),
        hbox({
            dynamics,
            separator() | color(Color::Black),
            calibration
        }),
        separator() | color(Color::Black),
        hbox({
            res_left,
            separator() | color(Color::Black),
            res_right
        }),
        separatorEmpty(),
    }) | size(WIDTH, EQUAL, 52);


}
