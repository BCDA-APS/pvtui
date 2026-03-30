#pragma once
#include <pv/caProvider.h>
#include <pva/client.h>

#include <pvtui/display_base.hpp>
#include <pvtui/pvtui.hpp>

class SmallMotorDisplay : public pvtui::DisplayBase {
  public:
    SmallMotorDisplay(pvtui::PVGroup &pvgroup, const std::string &record);
    ~SmallMotorDisplay() override = default;
    ftxui::Element get_renderer() override;
    ftxui::Component get_container() override;

  private:
    std::string record_name;
    pvtui::InputWidget desc;
    pvtui::InputWidget val;
    pvtui::ButtonWidget twr;
    pvtui::InputWidget twv;
    pvtui::ButtonWidget twf;
    pvtui::Monitor<std::string> rbv;
    pvtui::Monitor<int> dmov;
    pvtui::Monitor<int> lls;
    pvtui::Monitor<int> hls;
    pvtui::Monitor<int> lvio;
    pvtui::Monitor<std::string> egu;
    pvtui::ChoiceWidget able;
    pvtui::ChoiceWidget use_set;
    pvtui::ButtonWidget stop;
};


class MediumMotorDisplay : public pvtui::DisplayBase {
  public:
    MediumMotorDisplay(pvtui::PVGroup &pvgroup, const std::string &record);
    ~MediumMotorDisplay() override = default;
    ftxui::Element get_renderer() override;
    ftxui::Component get_container() override;

  private:
    std::string record_name;
    pvtui::InputWidget desc;
    pvtui::InputWidget val;
    pvtui::ButtonWidget twr;
    pvtui::InputWidget twv;
    pvtui::ButtonWidget twf;
    pvtui::Monitor<std::string> rbv;
    pvtui::Monitor<int> dmov;
    pvtui::Monitor<int> lls;
    pvtui::Monitor<int> hls;
    pvtui::Monitor<int> lvio;
    pvtui::Monitor<std::string> egu;
    pvtui::ChoiceWidget use_set;
    pvtui::Monitor<std::string> drbv;
    pvtui::InputWidget dval;
    pvtui::InputWidget hlm;
    pvtui::InputWidget dhlm;
    pvtui::InputWidget llm;
    pvtui::InputWidget dllm;
    pvtui::ChoiceWidget spmg;
    pvtui::ChoiceWidget able;
};

class AllMotorDisplay : public pvtui::DisplayBase {
  public:
    AllMotorDisplay(pvtui::PVGroup &pvgroup, const std::string &record);
    ~AllMotorDisplay() override = default;
    ftxui::Element get_renderer() override;
    ftxui::Component get_container() override;

  private:
    std::string record_name;
    pvtui::InputWidget desc;
    pvtui::InputWidget val;
    pvtui::ButtonWidget twr;
    pvtui::InputWidget twv;
    pvtui::ButtonWidget twf;
    pvtui::Monitor<std::string> rbv;
    pvtui::Monitor<int> dmov;
    pvtui::Monitor<int> lls;
    pvtui::Monitor<int> hls;
    pvtui::Monitor<int> lvio;
    pvtui::InputWidget egu;
    pvtui::ChoiceWidget use_set;
    pvtui::Monitor<std::string> drbv;
    pvtui::InputWidget dval;
    pvtui::InputWidget hlm;
    pvtui::InputWidget dhlm;
    pvtui::InputWidget llm;
    pvtui::InputWidget dllm;
    pvtui::ChoiceWidget spmg;
    pvtui::ChoiceWidget able;
    pvtui::InputWidget vmax;
    pvtui::InputWidget velo;
    pvtui::InputWidget vbas;
    pvtui::InputWidget accl;
    pvtui::InputWidget mres;
    pvtui::InputWidget eres;
    pvtui::InputWidget rres;
    pvtui::InputWidget rtry;
    pvtui::InputWidget off;
    pvtui::InputWidget prec;
    pvtui::InputWidget rlv;
    pvtui::InputWidget rval;
    pvtui::ChoiceWidget ueip;
    pvtui::ChoiceWidget urip;
    pvtui::ChoiceWidget dir;
    pvtui::ChoiceWidget cnen;
    pvtui::ChoiceWidget foff;
    pvtui::Monitor<std::string> rrbv;
};
