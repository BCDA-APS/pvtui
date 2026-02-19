#include "motor_display.hpp"
#include <pvtui/pvtui.hpp>
#include <ftxui/component/component.hpp>

ftxui::Decorator ColorDisabled = bgcolor(ftxui::Color::DarkRed) | color(ftxui::Color::Black);

SmallMotorDisplay::SmallMotorDisplay(pvtui::PVGroup &pvgroup, const pvtui::ArgParser &args)
    : pvtui::DisplayBase(pvgroup), args(args),
    desc(pvgroup, args, "$(P)$(M).DESC", pvtui::PVPutType::String),
    val(pvgroup, args, "$(P)$(M).VAL", pvtui::PVPutType::Double),
    twr(pvgroup, args, "$(P)$(M).TWR", " < "),
    twv(pvgroup, args, "$(P)$(M).TWV", pvtui::PVPutType::Double),
    twf(pvgroup, args, "$(P)$(M).TWF", " > "),
    rbv(pvgroup, args, "$(P)$(M).RBV"),
    dmov(pvgroup, args, "$(P)$(M).DMOV"),
    lls(pvgroup, args, "$(P)$(M).LLS"),
    hls(pvgroup, args, "$(P)$(M).HLS"),
    lvio(pvgroup, args, "$(P)$(M).LVIO"),
    egu(pvgroup, args, "$(P)$(M).EGU"),
    able(pvgroup, args, "$(P)$(M)_able", pvtui::ChoiceStyle::Horizontal),
    use_set(pvgroup, args, "$(P)$(M).SET", pvtui::ChoiceStyle::Horizontal),
    stop(pvgroup, args, "$(P)$(M).STOP", " STOP ")
{}

ftxui::Component SmallMotorDisplay::get_container() {
    using namespace ftxui;
    return Container::Vertical({
	desc.component(),
	val.component(),
	twr.component(),
	twv.component(),
	twf.component(),
	use_set.component(), stop.component()
    });
}

ftxui::Element SmallMotorDisplay::get_renderer() {
    using namespace ftxui;
    using namespace pvtui;

    auto val_bg = able.value().index == 1 ? ColorDisabled :
		lvio.value() == 1 ? bgcolor(Color::Yellow2) :
		EPICSColor::edit(val);

    return ftxui::vbox({
	desc.component()->Render()
	    | size(WIDTH, EQUAL, 20)
	    | underlined
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
	    val.component()->Render()
		| val_bg
		| size(WIDTH, EQUAL, 10)
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
    })
    | size(WIDTH, EQUAL, 24)
    | border
    | color(Color::Black)
    | center;
}

MediumMotorDisplay::MediumMotorDisplay(pvtui::PVGroup &pvgroup, const pvtui::ArgParser &args)
    : pvtui::DisplayBase(pvgroup), args(args),
    desc(pvgroup, args, "$(P)$(M).DESC", pvtui::PVPutType::String),
    val(pvgroup, args, "$(P)$(M).VAL", pvtui::PVPutType::Double),
    twr(pvgroup, args, "$(P)$(M).TWR", " < "),
    twv(pvgroup, args, "$(P)$(M).TWV", pvtui::PVPutType::Double),
    twf(pvgroup, args, "$(P)$(M).TWF", " > "),
    rbv(pvgroup, args, "$(P)$(M).RBV"),
    dmov(pvgroup, args, "$(P)$(M).DMOV"),
    lls(pvgroup, args, "$(P)$(M).LLS"),
    hls(pvgroup, args, "$(P)$(M).HLS"),
    lvio(pvgroup, args, "$(P)$(M).LVIO"),
    egu(pvgroup, args, "$(P)$(M).EGU"),
    use_set(pvgroup, args, "$(P)$(M).SET", pvtui::ChoiceStyle::Horizontal),
    drbv(pvgroup, args, "$(P)$(M).DRBV"),
    dval(pvgroup, args, "$(P)$(M).DVAL", pvtui::PVPutType::Double),
    hlm(pvgroup, args, "$(P)$(M).HLM", pvtui::PVPutType::Double),
    dhlm(pvgroup, args, "$(P)$(M).DHLM", pvtui::PVPutType::Double),
    llm(pvgroup, args, "$(P)$(M).LLM", pvtui::PVPutType::Double),
    dllm(pvgroup, args, "$(P)$(M).DLLM", pvtui::PVPutType::Double),
    spmg(pvgroup, args, "$(P)$(M).SPMG", pvtui::ChoiceStyle::Vertical),
    able(pvgroup, args, "$(P)$(M)_able", pvtui::ChoiceStyle::Horizontal)
{}

ftxui::Component MediumMotorDisplay::get_container() {
    using namespace ftxui;
    return Container::Vertical({
	desc.component(),
	hlm.component(),
	val.component(),
	llm.component(),
	dhlm.component(),
	dval.component(),
	dllm.component(),
	spmg.component(),
	twr.component(), twv.component(), twf.component(),
	able.component(), use_set.component(),
    });

}

ftxui::Element MediumMotorDisplay::get_renderer() {
    using namespace ftxui;
    using namespace pvtui;

    auto val_bg = able.value().index == 1 ? ColorDisabled :
		lvio.value() == 1 ? bgcolor(Color::Yellow2) :
		EPICSColor::edit(val);

    return ftxui::vbox({
	desc.component()->Render()
	    | EPICSColor::custom(desc, color(Color::Black) | bgcolor(Color::RGB(210,210,210)))
	    | size(WIDTH, EQUAL, 22) | center,
	separatorEmpty(),

	// 6 column hbox of vbox's
	// none | none | user | dial | lims/egu | spmg
	hbox({
	    filler() | size(WIDTH, EQUAL, egu.value().length()+8),
	    vbox({
		text("User") | center,
		hlm.component()->Render() | EPICSColor::edit(hlm),
		text(rbv.value())
		    | (use_set.value().index==0 ? EPICSColor::readback(use_set) : color(Color::Yellow2))
		    | (dmov.value() == 0 ? borderHeavy | color(Color::Green) : borderEmpty)
		    | center,
		val.component()->Render() | center | val_bg,
		separatorEmpty(),

		llm.component()->Render() | EPICSColor::edit(llm),
	    }) | size(WIDTH, EQUAL, 10),
	    separatorEmpty(),
	    vbox({
		text("Dial") | center,
		dhlm.component()->Render() | EPICSColor::edit(dhlm),
		text(drbv.value())
		    | (use_set.value().index==0 ? EPICSColor::readback(use_set) : color(Color::Yellow2))
		    | (dmov.value() == 0 ? borderHeavy | color(Color::Green) : borderEmpty)
		    | center,
		dval.component()->Render() | center | val_bg,
		separatorEmpty(),
		dllm.component()->Render() | EPICSColor::edit(dllm)
	    }),
	    separatorEmpty(),
	    vbox({
		separatorEmpty(),
		text(hls.value() ? unicode::rectangle(2) : "") | color(Color::Red),
		filler(),
		text(egu.value()) | EPICSColor::readback(egu),
		separatorEmpty(),
		text(lls.value() ? unicode::rectangle(2) : "") | color(Color::Red),
	    }) | size(WIDTH, EQUAL, egu.value().length()),
	    separatorEmpty(),
	    spmg.component()->Render()
		| EPICSColor::edit(spmg)
		| size(HEIGHT, EQUAL, 2),
	}) | center,

	separatorEmpty(),

	hbox({
	    twr.component()->Render() | EPICSColor::custom(twr, color(Color::Black)),
	    separatorEmpty(),
	    twv.component()->Render()
		| center
		| EPICSColor::edit(twv)
		| size(WIDTH, EQUAL, 11),
	    separatorEmpty(),
	    twf.component()->Render() | EPICSColor::custom(twf, color(Color::Black)),
	}) | center,

	separatorEmpty(),

	hbox({
	    separatorEmpty(),
	    use_set.component()->Render() | EPICSColor::edit(use_set),
	    separatorEmpty(),
	    able.component()->Render() | EPICSColor::edit(able),
	}) | center,
	separatorEmpty(),

    }) | center | EPICSColor::background();
}

AllMotorDisplay::AllMotorDisplay(pvtui::PVGroup &pvgroup, const pvtui::ArgParser &args)
    : pvtui::DisplayBase(pvgroup), args(args),
    desc(pvgroup, args, "$(P)$(M).DESC", pvtui::PVPutType::String),
    val(pvgroup, args, "$(P)$(M).VAL", pvtui::PVPutType::Double),
    twr(pvgroup, args, "$(P)$(M).TWR", " < "),
    twv(pvgroup, args, "$(P)$(M).TWV", pvtui::PVPutType::Double),
    twf(pvgroup, args, "$(P)$(M).TWF", " > "),
    rbv(pvgroup, args, "$(P)$(M).RBV"),
    dmov(pvgroup, args, "$(P)$(M).DMOV"),
    lls(pvgroup, args, "$(P)$(M).LLS"),
    hls(pvgroup, args, "$(P)$(M).HLS"),
    lvio(pvgroup, args, "$(P)$(M).LVIO"),
    egu(pvgroup, args, "$(P)$(M).EGU", pvtui::PVPutType::String),
    use_set(pvgroup, args, "$(P)$(M).SET", pvtui::ChoiceStyle::Horizontal),
    drbv(pvgroup, args, "$(P)$(M).DRBV"),
    dval(pvgroup, args, "$(P)$(M).DVAL", pvtui::PVPutType::Double),
    hlm(pvgroup, args, "$(P)$(M).HLM", pvtui::PVPutType::Double),
    dhlm(pvgroup, args, "$(P)$(M).DHLM", pvtui::PVPutType::Double),
    llm(pvgroup, args, "$(P)$(M).LLM", pvtui::PVPutType::Double),
    dllm(pvgroup, args, "$(P)$(M).DLLM", pvtui::PVPutType::Double),
    spmg(pvgroup, args, "$(P)$(M).SPMG", pvtui::ChoiceStyle::Vertical),
    able(pvgroup, args, "$(P)$(M)_able", pvtui::ChoiceStyle::Vertical),
    vmax(pvgroup, args, "$(P)$(M).VMAX", pvtui::PVPutType::Double),
    velo(pvgroup, args, "$(P)$(M).VELO", pvtui::PVPutType::Double),
    vbas(pvgroup, args, "$(P)$(M).VBAS", pvtui::PVPutType::Double),
    accl(pvgroup, args, "$(P)$(M).ACCL", pvtui::PVPutType::Double),
    mres(pvgroup, args, "$(P)$(M).MRES", pvtui::PVPutType::Double),
    eres(pvgroup, args, "$(P)$(M).ERES", pvtui::PVPutType::Double),
    rres(pvgroup, args, "$(P)$(M).RRES", pvtui::PVPutType::Double),
    rtry(pvgroup, args, "$(P)$(M).RTRY", pvtui::PVPutType::Integer),
    off(pvgroup, args, "$(P)$(M).OFF", pvtui::PVPutType::Double),
    prec(pvgroup, args, "$(P)$(M).PREC", pvtui::PVPutType::Integer),
    rlv(pvgroup, args, "$(P)$(M).RLV", pvtui::PVPutType::Double),
    rval(pvgroup, args, "$(P)$(M).RVAL", pvtui::PVPutType::Double),
    ueip(pvgroup, args, "$(P)$(M).UEIP", pvtui::ChoiceStyle::Horizontal),
    urip(pvgroup, args, "$(P)$(M).URIP", pvtui::ChoiceStyle::Horizontal),
    dir(pvgroup, args, "$(P)$(M).DIR", pvtui::ChoiceStyle::Horizontal),
    cnen(pvgroup, args, "$(P)$(M).CNEN", pvtui::ChoiceStyle::Horizontal),
    foff(pvgroup, args, "$(P)$(M).FOFF", pvtui::ChoiceStyle::Dropdown),
    rrbv(pvgroup, args, "$(P)$(M).RRBV")
{}

ftxui::Component AllMotorDisplay::get_container() {
    using namespace ftxui;
    return Container::Vertical({
	desc.component(),
	hlm.component(),
	val.component(),
	llm.component(),
	rlv.component(),
	dhlm.component(),
	dval.component(),
	dllm.component(),
	rval.component(),
	able.component(),
	spmg.component(),
	twr.component(),
	twv.component(),
	twf.component(),
	vmax.component(),
	velo.component(),
	vbas.component(),
	accl.component(),
	foff.component(),
	use_set.component(),
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
    });
}

ftxui::Element AllMotorDisplay::get_renderer() {
    using namespace ftxui;
    using namespace pvtui;

    auto val_bg = able.value().index == 1 ? ColorDisabled :
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
	text("(" + args.macros.at("P") + args.macros.at("M") + ")")
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
