# lcdkit-base-lib.pl
#use packages
use FindBin qw($Bin);
use lib "$Bin/../../../../../../vendor/huawei/extra/kernel/drivers/lcd/tools/lib";
use lib "$Bin/../../../../../../vendor/huawei/extra/kernel/drivers/lcd/tools/share";
use XML::LibXML;
use IO::Handle;
use warnings;
use strict;
use File::Spec;
use File::Basename;


# initialize the parser
my $xml_parser = new XML::LibXML;
# open a filehandle and parse
my $parser_handle = new IO::Handle;

#print info prefix
my $debug_print_switch = 0;
my $debug_info = "[lcdkit][debug]";
my $error_info = "[lcdkit][error]";
my $warning_info = "[lcdkit][warning]";
my $parse_error_string = "isdfjhadfhuejhafuiha565121w658923893uhfj";

my $invalid_panel = "INVALID_PANEL_ID";
my $default_panel = "DEFAULT_AUO_OTM1901A_5P2_1080P_VIDEO_DEFAULT_PANEL";


debug_print("devkit tool get lib bin path is: $Bin!\n");

my @string_trans_map = (
            [ "\"rgb_swap_rgb\"",          0            ],
            [ "\"rgb_swap_rbg\"",          1            ],
            [ "\"rgb_swap_bgr\"",          2            ],
            [ "\"rgb_swap_brg\"",          3            ],
            [ "\"rgb_swap_grb\"",          4            ],
            [ "\"rgb_swap_gbr\"",          5            ],
            [ "\"burst_mode\"",            2            ],
            [ "\"non_burst_sync_event\"",  1            ],
            [ "\"non_burst_sync_pulse\"",  0            ],
            [ "\"tight\"",                 0            ],
            [ "\"loose\"",                 1            ],
            [ "\"lane_map_0123\"",         0            ],
            [ "\"lane_map_3012\"",         1            ],
            [ "\"lane_map_2301\"",         2            ],
            [ "\"lane_map_1230\"",         3            ],
            [ "\"lane_map_0321\"",         4            ],
            [ "\"lane_map_1032\"",         5            ],
            [ "\"lane_map_2103\"",         6            ],
            [ "\"lane_map_3210\"",         7            ],
            [ "\"none\"",                  0            ],
            [ "\"trigger_sw\"",            4            ],
            [ "\"mdss_dsi0\"",             "\"dsi:0:\"" ],
            [ "\"mdss_dsi1\"",             "\"dsi:1:\"" ]);
            

sub get_err_string { return $parse_error_string; }
sub error_print { print "\n" . $error_info . shift; }
sub warning_print { print "\n" . $warning_info . shift; }
sub debug_print { if ($debug_print_switch eq 1) { print "\n" . $debug_info . shift; } }

sub get_file_name { return lc(shift) . '-' . lc(shift); }

sub get_out_dtsi_path  {  return shift . '/' . shift . '.dtsi'; }
sub get_out_head_path  {  return shift . '/' . shift . '.h'; }
sub get_out_effect_path  {  return shift . '/' . shift . '.h'; }

#0: lcd; 1: oled
sub parse_panel_type { return shift eq "0x01" ? 0 : 1; }

sub exit_with_info
{
    welcome_print_end(shift);
    exit shift;
}

sub welcome_print_end
{
    my $file_name = shift;
	  debug_print "\n\n";
	  debug_print "parse $file_name panel end.\n";
	  debug_print "========================****************************========================\n";
	  debug_print "=====================**********see you later!*********======================\n";
	  debug_print "========================****************************========================\n";
    debug_print "\n\n";
}


sub welcome_print_begin
{
    my $i = 0;
    my $xml_files = shift;
    my $file_name = shift;
    my @xml_files = @$xml_files;
    debug_print "\n\n";
    debug_print "========================****************************========================\n";
    debug_print "=================********welcome to lcd xml parse!!**********===============\n";
    debug_print "========================****************************========================\n";
    debug_print "parse $file_name panel use xml files: \n\n";
    foreach my $lc_xml (@xml_files) {
        debug_print"[$i]----$lc_xml \n\n";
        $i++;
    }
    debug_print "now parsing......\n\n";
}

sub print_data_var
{
    my $xml_node = shift;
    my $var_string = parse_multi_xml($xml_node);
    if ($var_string eq $parse_error_string)
    {
        error_print("fail to parse data var: $xml_node!\n");
        return $parse_error_string;
    }
    return " = " . $var_string . ";\n\n";
}


sub file_name_from_path
{
    my $file_path = shift;
    my $file_name = $file_path;
    $file_path = dirname($file_path);
    $file_name =~ s/$file_path//g;
    $file_name =~ s/\///g;
    $file_name =~ s/-/_/g;

    my @element = split /\./, $file_name;
    $file_name = $element[0];

    return lc($file_name);
}


sub get_parse_xml_para
{
    my $xml_files = shift;
    my @xml_files = @$xml_files;

    my $para_string = '';
    my $xml_file_num = 0;
    foreach my $xml_file (@xml_files)    {
        debug_print("parse xml file: $xml_file\n");
        $xml_file_num++;
        $para_string = $para_string . ' ' . $xml_file;
    }

    #debug_print("get parse para string: $parse_para_num\n");
    $para_string = $xml_file_num . ' ' . $para_string;
    
    return $para_string;
}


sub get_module_map_list
{
    my $xml_md = shift;
    my $xml_list = shift;
    my $xml_node = shift;
    my $list_entry = "";
    my @module_map_list;

    my $xml_file = get_xml_file($xml_list, '\w+'. $xml_md . '.xml');
    if ($xml_file eq $parse_error_string) {
        error_print("cann't find plat $xml_md xml files!\n");
        return @module_map_list;
    }

    my $xmldoc = $xml_parser->parse_file($xml_file);
    for my $dest($xmldoc->findnodes($xml_node)) {
        $list_entry = $dest->textContent();

        my $tmp_num = 0;
        foreach my $tmp (@module_map_list) {
            if ($tmp eq $list_entry){
                $tmp_num++;
            }
        }

        if ($tmp_num eq 0) {
            push(@module_map_list, $list_entry);
        }
    }

    return @module_map_list;
}


#find xml by patten
sub get_xml_file
{
    my $lc_xml_files = shift;
    my $match_patten = shift;
    my @lc_xml_files = @$lc_xml_files;

    my $xml_num = 0;
    my $match_xml = $parse_error_string;
    #$match_patten = '\w+' . $match_patten;
    foreach my $xml_file (@lc_xml_files) {
        if ($xml_file =~ m/$match_patten/) {
            $xml_num++;
            $match_xml = $xml_file;
            debug_print("get_xml_file get match xml file: $xml_file\n");
        }
    }

    if ($xml_num ne 1)    {
        $match_xml = $parse_error_string;
        debug_print("get_xml_file get match $match_patten xml file fail! match num: $xml_num\n");
    }

    return $match_xml;
}


sub get_platform_attr
{
    my $xml_md = shift;
    my $xml_list = shift;
    my $xml_node = shift;
    
    my $xml_file = get_xml_file($xml_list, '\w+'. $xml_md . '.xml');
    if ($xml_file eq $parse_error_string) {
        error_print("cann't find plat $xml_md xml files!\n");
        return $parse_error_string;
    }

    my $xmldoc = $xml_parser->parse_file($xml_file);
    for my $dest($xmldoc->findnodes($xml_node)) {
        return $dest->textContent();
    }

    return $parse_error_string;
}


sub parse_single_xml
{
    my $xml_fh = shift;
    my $xml_node = shift;

    for my $property($xml_fh->findnodes($xml_node))
    {
        return $property->textContent();
    }

    return $parse_error_string;
}

sub parse_multi_xml_base
{
    my $xml_files = shift;
    my @xml_files = @$xml_files;
    
    my $xml_node = shift;

    foreach my $lc_xml (@xml_files) {
        my $parse_str = parse_single_xml($lc_xml, $xml_node);
        if ($parse_str ne $parse_error_string)    {
            return $parse_str;
        }
    }

    #debug_print("");
    return $parse_error_string;
}

sub create_dtsi_tail    {    return "    };\n};";    }
sub create_dtsi_header
{
    my $file_name = shift;
    my $xml_version = shift;

    my $dtsi_header;
    $dtsi_header  = "/* Copyright (c) 2013, The Linux Foundation. All rights reserved.\n";
    $dtsi_header .= " *\n";
    $dtsi_header .= " * This program is free software; you can redistribute it and/or modify\n";
    $dtsi_header .= " * it under the terms of the GNU General Public License version 2 and\n";
    $dtsi_header .= " * only version 2 as published by the Free Software Foundation.\n";
    $dtsi_header .= " *\n";
    $dtsi_header .= " * This program is distributed in the hope that it will be useful,\n";
    $dtsi_header .= " * but WITHOUT ANY WARRANTY; without even the implied warranty of\n";
    $dtsi_header .= " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n";
    $dtsi_header .= " * GNU General Public License for more details.\n";
    $dtsi_header .= " */\n\n";
    $dtsi_header .= "/*-------------------------------------------------------------------------\n";
    $dtsi_header .= " * This file is autogenerated file using gcdb parser. Please do not edit it.\n";
    $dtsi_header .= " * Update input XML file to add a new entry or update variable in this file\n";
    $dtsi_header .= " * VERSION = " . $xml_version . "\n";
    $dtsi_header .= " *-----------------------------------------------------------------------*/\n";

    $dtsi_header .= "/ {\n";
    $dtsi_header .= "\tdsi_" . lc($file_name) . ": lcdkit_" . lc($file_name) . " {\n";

    return $dtsi_header;
}

sub parser_hex_order_base
{    
    my $patten = '([^"]*)';
    (my $parser_string) = shift =~ /"$patten"/;
    $parser_string =~ s/,//g;
    
    my $platform = shift;
    if ($platform eq 'qcom')
    {
        $parser_string =~ s/0x//g;
        $parser_string = '[' . $parser_string . ']';
    }
    else
    {
        $parser_string = '<' . $parser_string . '>';
    }
    return $parser_string;
}

sub parser_cmd_state { return shift eq "0" ? "dsi_lp_mode" : "dsi_hs_mode"; }
sub parser_panel_cmd_type { return shift eq "0" ? "dsi_video_mode" : "dsi_cmd_mode"; }

sub parser_pmic_ctrl
{
    my $tmp = shift;
    my @bltmp = ("bl_ctrl_pwm", "bl_ctrl_wled", "bl_ctrl_dcs", "bl_ctrl_ic_ti");
    return $tmp < 4 ? $bltmp[$tmp] : "error_bl_type";
}

sub apply_patten
{
    my $patten = '([^"]*)';
    (my $parse_string) = shift =~ /"$patten"/;
    if (!$parse_string)
    {
        error_print("apply_patten apply patten failed!\n");
        return $parse_error_string;
    }
    return $parse_string;
}

sub create_hfile_tail { return "\n#endif /*_PANEL_" . uc(shift) . "_H_*/\n"; }

sub create_hfile_header
{
    my $xml_version = shift;
    my $panel_name = shift;
    my $hfile_header;
    $hfile_header  = "/* Copyright (c) 2013, The Linux Foundation. All rights reserved.\n";
    $hfile_header .= " *\n";
    $hfile_header .= " * Redistribution and use in source and binary forms, with or without\n";
    $hfile_header .= " * modification, are permitted provided that the following conditions\n";
    $hfile_header .= " * are met:\n";
    $hfile_header .= " *  * Redistributions of source code must retain the above copyright\n";
    $hfile_header .= " *    notice, this list of conditions and the following disclaimer.\n";
    $hfile_header .= " *  * Redistributions in binary form must reproduce the above copyright\n";
    $hfile_header .= " *    notice, this list of conditions and the following disclaimer in\n";
    $hfile_header .= " *    the documentation and/or other materials provided with the\n";
    $hfile_header .= " *    distribution.\n";
    $hfile_header .= " *  * Neither the name of The Linux Foundation nor the names of its\n";
    $hfile_header .= " *    contributors may be used to endorse or promote products derived\n";
    $hfile_header .= " *    from this software without specific prior written permission.\n";
    $hfile_header .= " *\n";
    $hfile_header .= " * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n";
    $hfile_header .= " * \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n";
    $hfile_header .= " * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS\n";
    $hfile_header .= " * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE\n";
    $hfile_header .= " * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\n";
    $hfile_header .= " * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,\n";
    $hfile_header .= " * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS\n";
    $hfile_header .= " * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED\n";
    $hfile_header .= " * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n";
    $hfile_header .= " * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT\n";
    $hfile_header .= " * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n";
    $hfile_header .= " * SUCH DAMAGE.\n";
    $hfile_header .= " */\n";
    $hfile_header .= "\n";
    $hfile_header .= "/*-------------------------------------------------------------------------\n";
    $hfile_header .= " * This file is autogenerated file using gcdb parser. Please do not edit it.\n";
    $hfile_header .= " * Update input XML file to add a new entry or update variable in this file\n";
    $hfile_header .= " * VERSION = " . $xml_version . "\n";
    $hfile_header .= " *-----------------------------------------------------------------------*/\n";

    $hfile_header .= "\n#ifndef _PANEL_". uc($panel_name) . "_H_\n";
    $hfile_header .= "\n#define _PANEL_". uc($panel_name) . "_H_\n";

    $hfile_header .= hfile_section_header("HEADER files");

    return $hfile_header;
}


sub hfile_section_header
{
    my $section = shift;
    my $parse_str =  "/*-----------------------------------------------------------------------*/\n";
    $parse_str .= "/* " . $section;
    for (my $i = length($section); $i < 70; $i++) { $parse_str .= " ";    }
    $parse_str .= "*/\n";
    $parse_str .= "/*-----------------------------------------------------------------------*/\n";

    return $parse_str;
}


sub create_data_struct
{
    my $tmp_count = 0;
    my @data_struct_step;
    my $struct_attr = shift;
    my @struct_attr = @$struct_attr;
    my $parser_string = "= {\n        ";

    for ($tmp_count = 0; $tmp_count < @struct_attr; $tmp_count++)
    {
        my $parse_step = "";
        my $dsi_reset_step = "";
        if ($struct_attr[$tmp_count][0] eq "panel_name")
        {
            $parse_step = $struct_attr[$tmp_count][2];
        }
        else
        {
            $parse_step = parse_multi_xml($struct_attr[$tmp_count][0]);
        }

        if ($parse_step eq $parse_error_string)    {
            $parse_step = $struct_attr[$tmp_count][4];
            if ($struct_attr[$tmp_count][3] ne '-w') {
                warning_print("set $struct_attr[$tmp_count][0] value to default!\n");
            }            
        }

        if ($struct_attr[$tmp_count][1] eq "-f")
        {
            $parse_step = $struct_attr[$tmp_count][2]->($parse_step);
            
            if ($parse_step eq $parse_error_string)
            {
                error_print("create_data_struct process parse string failed!\n");
            }
        }

        $parse_step .= ($tmp_count < (@struct_attr - 1)) ? "," : "";

        push(@data_struct_step, $parse_step);
    }

    $tmp_count = 1;
    foreach my $parser_tmp (@data_struct_step) {
        if (length($parser_string) > (70 * $tmp_count)) {
            $parser_string .= "\n        ";
            $tmp_count++;
        }

        $parser_string .= $parser_tmp . " ";
    }
    return $parser_string . "\n};\n\n";
}


sub string_tran
{
    my $target_str = shift;
    my $tmp_count = 0;

    for ($tmp_count = 0; $tmp_count < @string_trans_map; $tmp_count++)
    {
        if ($string_trans_map[$tmp_count][0] eq $target_str) {
            return $string_trans_map[$tmp_count][1];
        }
    }

    error_print("transmit string $target_str fail!\n\n");
    return $parse_error_string;
}


sub qcom_cmd_struct_grp
{
    my $patten = '([^"]*)';
    my $struct_attr = shift;
    my @struct_attr = @$struct_attr;

    my $tmp_string = parse_multi_xml($struct_attr[0]);
    if ($tmp_string eq $parse_error_string)    {
        debug_print("qcom_cmd_struct_grp fail to parse $struct_attr[0] !\n\n");
        return "static struct mipi_dsi_cmd " . $struct_attr[2] . "[] = { };\n\n";
    }

    (my $element) = $tmp_string =~ /"$patten"/;
    my @lines = split /\n/, $element;
    my $parse_string = "";
    my $i = 0;
    foreach my $line (@lines)
    {
        my @sep = split /,/, $line;
        $parse_string .= "static char " . $struct_attr[1] . $i . "[] = {\n";
        $parse_string .= "    ";
        if(scalar @sep > 7)
        {
            my $cmdlen = $sep[6];
            my $cmdtype = $sep[0];
            $cmdtype =~ s/ //g;
            $cmdtype =~ s/\t//g;
            $cmdlen =~ s/ //g;
            $cmdlen =~ s/\t//g;
            if($cmdtype eq "0x29" || $cmdtype eq "0x39")
            {
                $parse_string .=  $cmdlen . ", 0x00, " . $cmdtype . ", 0xC0,\n    ";
            }
            my $j = 0;

            for(my $i = 7; $i < scalar @sep; $i++)
            {
                my $tmp = $sep[$i];
                $tmp =~ s/ //g;
                $tmp =~ s/\t//g;
                if($tmp ne "")
                {
                    $parse_string  .= $tmp . ", ";
                    $j++;
                }
                if (($j % 4) == 0)
                {
                    chop($parse_string);
                    $parse_string .= "\n    ";
                }
            }
            if($cmdtype eq "0x29" || $cmdtype eq "0x39")
            {
                if ( ($j % 4) ne 0)
                {
                    for( ; ($j % 4) ne 0 ; $j++)
                    {
                        $parse_string .= ($j % 4) eq 3 ? "0xFF " : "0xFF, ";
                    }
                    chop($parse_string);
                    #$parse_string .= "\n";
                }
                else
                {
                    chop($parse_string);
                }
            }
            else
            {
                for( ; ($j % 2) ne 0 ; $j++)
                {
                    $parse_string .= ($j % 4) eq 1 ? "0xFF" : "0xFF, ";
                }
                $parse_string .= $cmdtype . ", 0x80";
            }
            $parse_string .= "};\n"
        }
        $i++;

        $parse_string .= "\n\n";
    }

    $parse_string .= "\n\n";

    $parse_string .= "static struct mipi_dsi_cmd " . $struct_attr[2] . "[] = {\n";
    my $bool = 0;
    $i = 0;

    foreach my $line (@lines)
    {
        my @sep = split /,/, $line;

        #$parse_string .= "    ";

        if(scalar @sep > 7)
        {
            my $cmdtype = $sep[0];
            $cmdtype =~ s/ //g;
            $cmdtype =~ s/\t//g;

            my $cmdsize = 0;
            my $hexsize = 0;

            if($cmdtype eq "0x29" || $cmdtype eq "0x39")
            {
                my $j = 0;
                for(my $i = 7; $i < scalar @sep; $i++)
                {
                    my $tmp = $sep[$i];
                    $tmp =~ s/ //g;
                    $tmp =~ s/\t//g;
                    if($tmp ne "")
                    {
                        $cmdsize += 1;
                        $j++;
                    }
                }
                for( ; ($j % 4) ne 0 ; $j++)
                {
                    $cmdsize += 1;
                }

                # calculate the correct size of command
                $hexsize = sprintf("{0x%x, ", $cmdsize + 4);
            }
            else
            {
                $hexsize = sprintf("{0x%x, ", 4);
            }

            $parse_string .=  $hexsize;

            $parse_string .= $struct_attr[1] . $i . "," . $sep[4] . ($i eq @lines-1 ? "}" : "},");

            $i++;
        }

        $parse_string .= "\n";
    }

    chop($parse_string);
    $parse_string .= "\n};\n#define " . uc($struct_attr[2]) . " " . $i . "\n\n\n";

    return $parse_string;
}


sub hisi_cmd_struct_grp
{
    my $patten = '([^"]*)';
    my $struct_attr = shift;
    my @struct_attr = @$struct_attr;

    my $tmp_string = parse_multi_xml($struct_attr[0]);
    if ($tmp_string eq $parse_error_string)    {
        debug_print("hisi_cmd_struct_grp fail to parse $struct_attr[0] !\n\n");
        return "static struct dsi_cmd_desc " . $struct_attr[1] . "[] = { };\n\n";
    }
    
    (my $element) = $tmp_string =~ /"$patten"/;
    my $parse_string = "";

    my @lines = split /\n/, $element;
    my $toPrint = "";
    my $i = 0;
    foreach my $line (@lines)
    {
        my @sep = split /,/, $line;
        $parse_string .= "static char " . $struct_attr[1] . $i . "[] = {\n";
        
        
        $toPrint = "";
        $toPrint = "        ";
        if(scalar @sep > 7)
        {
            my $cmdlen = $sep[6];
            my $cmdtype = $sep[0];
            $cmdtype =~ s/ //g;
            $cmdtype =~ s/\t//g;
            $cmdlen =~ s/ //g;
            $cmdlen =~ s/\t//g;
        
            my $j = 0;
            for(my $i = 7; $i < scalar @sep; $i++)
            {
                my $tmp = $sep[$i];
                $tmp =~ s/ //g;
                $tmp =~ s/\t//g;
                if($tmp ne "")
                {
                    if($i == (scalar @sep - 1))
                    {
                        $toPrint  .= $tmp . ",";
                    }
                    else
                    {
                        $toPrint  .= $tmp . ", ";
                    }
                    $j++;
                }
                if ($i == 7 && $i != (scalar @sep - 1))
                {
                    chop($toPrint);
                    $toPrint .= "\n";
                    $toPrint .= "        ";
                }	
            }
            $toPrint .= "\n};\n"
        }
        $i++;
        
        #$toPrint .= "\n\n";
        $parse_string .= $toPrint;
    }

    $parse_string .= "\nstatic struct dsi_cmd_desc " . $struct_attr[1] . "[] = {\n";
    my $bool = 0;
    $toPrint = "";
    $i = 0;
    
    foreach my $line (@lines)
    {
        my @sep = split /,/, $line;
        $toPrint .= "    {\n";
        if(scalar @sep > 6)
        {
            my $cmdtype = $sep[0];
            my $waittype = $sep[5];
            $cmdtype =~ s/ //g;
            $cmdtype =~ s/\t//g;
            $waittype =~ s/ //g;
            $waittype =~ s/\t//g;
            
            my $cmdsize = 0;
            my $dtype = "";
            my $vc = 0;
            my $delaytype = "";
            my $cmdsleep = $sep[4];			
            $cmdsleep =~ s/ //g;
            $cmdsleep =~ s/\t//g;
            
            if($cmdtype eq "0x39")
            {
                $dtype = "DTYPE_DCS_LWRITE, ";
            }
            elsif($cmdtype eq "0x29")
            {
                $dtype = "DTYPE_GEN_LWRITE, ";
            }
            elsif($cmdtype eq "0x15")
            {
                $dtype = "DTYPE_DCS_WRITE1, ";
            }
            elsif($cmdtype eq "0x05")
            {
                $dtype = "DTYPE_DCS_WRITE, ";
            }
            elsif($cmdtype eq "0x06")
            {
                $dtype = "DTYPE_DCS_READ, ";
            }
            elsif($cmdtype eq "0x14")
            {
                $dtype = "DTYPE_GEN_READ1, ";
            }
            else
            {
                $dtype = "DTYPE_GEN_LWRITE, ";
            }
            
            if($waittype eq "0x01")
            {
                $delaytype = "WAIT_TYPE_MS,";
            }
            else
            {
                $delaytype = "WAIT_TYPE_US,";
            }
            $toPrint .= "        ";
            $toPrint .= $dtype;
            $toPrint .= $vc . ", ";
            $toPrint .= $cmdsleep . ", ";
            $toPrint .= $delaytype;
            $toPrint .= "\n";
            $toPrint .= "        sizeof(" . $struct_attr[1] . $i
                                 . "),\n        " . $struct_attr[1] . $i;
            $toPrint .= "\n";
            $toPrint .= "    },\n";
            $i++;
        }
    }
    
    $parse_string .= $toPrint . "};\n\n";
    
    return $parse_string;
}


sub parser_reset_seq_comm
{
    my $parser_string = "";
    my $struct_attr = shift;
    my @struct_attr = @$struct_attr;
    
    my @parser_reset_seq;

    my $dsi_reset_step = "{ 1, 0, 1, }, ";
    push(@parser_reset_seq, $dsi_reset_step);

    $dsi_reset_step = "{ ";
    my $tmp_count = 0;
    for ($tmp_count = 0; $tmp_count < 3; $tmp_count++)
    {
        my $parse_tmp = parse_multi_xml($struct_attr[$tmp_count][0]);
        if ($parse_tmp eq $parse_error_string)
        {
            error_print("fail to parse data var: $struct_attr[$tmp_count][0] !\n");
            return $parse_error_string;
        }
        $dsi_reset_step .= $parse_tmp . ', ';
    }
    $dsi_reset_step .= '}, ';
    push(@parser_reset_seq, $dsi_reset_step);

    $dsi_reset_step = parse_multi_xml($struct_attr[$tmp_count][0]);
    if ($dsi_reset_step eq $parse_error_string)
    {
        error_print("fail to parse data var: $struct_attr[$tmp_count][0] !\n");
        return $parse_error_string;
    }
    push(@parser_reset_seq, $dsi_reset_step);

    foreach my $parser_tmp (@parser_reset_seq) {
        $parser_string .=  $parser_tmp;
    }

    $parser_string = "= {\n    " . $parser_string . "\n};\n\n";

    return $parser_string;
}

sub parser_reset_seq_qcom
{
    my $parser_string = "";
    my $struct_attr = shift;
    my @struct_attr = @$struct_attr;
    my $xml_node = $struct_attr[0];

    my @parser_reset_seq;

    my $dsi_reset_step = "{ ";
    for (my $tmp_count = 1; $tmp_count <= 3; $tmp_count++)
    {
        my $parse_tmp = parse_multi_xml($xml_node . "/PinState" . $tmp_count);
        if ($parse_tmp eq $parse_error_string)
        {
            error_print("fail to parse data var: $xml_node !\n");
            return $parse_error_string;
        }
        $dsi_reset_step .= $parse_tmp . ', ';
    }
    $dsi_reset_step .= '}, ';
    push(@parser_reset_seq, $dsi_reset_step);

    $dsi_reset_step = "{ ";
    for (my $tmp_count = 1; $tmp_count <= 3; $tmp_count++)
    {
        my $parse_tmp = parse_multi_xml($xml_node . "/PulseWidth" . $tmp_count);
        if ($parse_tmp eq $parse_error_string)
        {
            error_print("fail to parse data var: $xml_node !\n");
            return $parse_error_string;
        }
        $dsi_reset_step .= $parse_tmp . ', ';
    }
    $dsi_reset_step .= '}, ';
    push(@parser_reset_seq, $dsi_reset_step);

    $dsi_reset_step = parse_multi_xml($xml_node . "/EnableBit");
    if ($dsi_reset_step eq $parse_error_string)
    {
        error_print("fail to parse data var: $xml_node !\n");
        return $parse_error_string;
    }
    push(@parser_reset_seq, $dsi_reset_step);

    foreach my $parser_tmp (@parser_reset_seq) {
        $parser_string .=  $parser_tmp;
    }

    $parser_string = $struct_attr[1] . " ={\n    " . $parser_string . "\n};\n\n";

    return $parser_string;
}


sub add_get_addr    {    return '&' . shift;    }

sub parse_gpios
{
    my $gpios = '';
    my @gpios = split /,/, shift;
    foreach my $gpio (@gpios)
    {
        my $temp = apply_patten($gpio);
        $gpios .= '<' . add_get_addr($temp) . '>,';
    }
    
    chop($gpios);
    return $gpios;
}
sub pin_func_trans
{
    my $pin_funcs = shift;
    $pin_funcs = apply_patten($pin_funcs);
    $pin_funcs =~ s/ //g;
    $pin_funcs =~ s/,/ &/g;
    $pin_funcs = '&' . $pin_funcs;
	
    return $pin_funcs;
}

sub pin_name_trans
{
    my $pin_name = shift;
    $pin_name =~ s/ //g;
    $pin_name =~ s/,/","/g;
    return $pin_name;
}

sub add_prefix
{
    my $parse_string = apply_patten(shift);
    return "\"/"."dss_" . $parse_string . "\"";
}

#sub print_dsi_ctrl { return shift eq "\"mdss_dsi0\"" ? "\"dsi:0:\"" : "\"dsi:1:\""; }

sub create_effect_lut
{
    my $property = shift;
    my $parse_string = parse_multi_xml($property);
    if ($parse_string eq $parse_error_string)    {
        error_print("create_effect_lut fail to parse $property!\n\n");
        return $parse_error_string;
    }
    
    
    $parse_string = apply_patten($parse_string);
    if ($parse_string eq $parse_error_string)
    {
        error_print("create_effect_lut process parse string failed!\n");
        return $parse_error_string;
    }
    
    my @lines = split /\n/, $parse_string;
    my $toPrint = "= {\n    ";
    my $i = 0;
       
    foreach my $line (@lines)
    {
        my @sep = split /,/, $line;
        for(my $i = 0; $i < scalar @sep; $i++)
        {
            my $tmp = $sep[$i];
            $tmp =~ s/ //g;
            $tmp =~ s/\t//g;
            if($tmp ne "")
            {
                if($i == (scalar @sep - 1))
                {
                    $toPrint .= $tmp . ",";
                }
                else
                {
                    $toPrint .= $tmp . ", ";
                }
            }
            if($i == 15)
            {
                chop($toPrint);
                $toPrint .= ",";
                $toPrint .= "\n";
                $toPrint .= "    ";
            }
        }
    }
    chop($toPrint);
    $toPrint .= "};\n";
    return $toPrint;
}

sub create_effect_lut_grp
{
    my $property = shift;
    my $parse_string = parse_multi_xml($property);
    if ($parse_string eq $parse_error_string)    {
        error_print("create_effect_lut_grp fail to parse $property!\n\n");
        return $parse_error_string;
    }
        
    $parse_string = apply_patten($parse_string);
    if ($parse_string eq $parse_error_string)
    {
        error_print("create_effect_lut_grp process parse string failed!\n");
        return $parse_error_string;
    }
    
    my @lines = split /\n/, $parse_string;
    my $toPrint = "";
    my $i = 0;
	my $row = 0;
	
	$toPrint .= " = {\n";
	foreach my $line (@lines)
	{
		my @sep = split /,/, $line;
		
		if(($row % 9) == 0)
		{
			$toPrint .= "    {";
			$toPrint .= "\n";
			$toPrint .= "        {";
		}
		else
		{
			$toPrint .= "        {";
		}

		for(my $i = 0; $i < scalar @sep; $i++)
		{
			my $tmp = $sep[$i];
			$tmp =~ s/ //g;
			$tmp =~ s/\t//g;

			if($tmp ne "")
			{
				if($i == (scalar @sep - 1))
				{
					$toPrint .= $tmp . ",";
				}
				else
				{
					$toPrint .= $tmp . ", ";
				}
			}	
			if($i == 8)
			{
				$toPrint .= "},";
				$toPrint .= "\n";
			}				
		}
		$row++;		
		if(($row % 9) == 0)
		{
			$toPrint .= "    ";
			$toPrint .= "},";
			$toPrint .= "\n";
		}		
	}
	#chop($toPrint);
	$toPrint .= "};\n";
	return $toPrint;
}

sub parser_small_trans { return lc(shift); }

sub parser_dsi_reset_seq
{
    my $parser_string = "";
    my $xml_node = shift;
    my @parser_dsi_reset_seq;

    for (my $tmp_count = 1; $tmp_count <= 3; $tmp_count++)
    {
        my $dsi_reset_step = "";
        my $parse_tmp = parse_multi_xml($xml_node . "/PinState" . $tmp_count);
        if ($parse_tmp eq $parse_error_string)    {
            error_print("fail to parse PinState!\n");
            return $parse_error_string;
        }
        $dsi_reset_step = '<' . $parse_tmp . ' ';

        $parse_tmp = parse_multi_xml($xml_node . "/PulseWidth" . $tmp_count);
        if ($parse_tmp eq $parse_error_string)    {
            error_print("fail to parse PulseWidth!\n");
            return $parse_error_string;;
        }
        $dsi_reset_step = $dsi_reset_step . $parse_tmp . '>';

        push(@parser_dsi_reset_seq, $dsi_reset_step);
    }

    foreach my $parser_tmp (@parser_dsi_reset_seq) {
        $parser_string .=  ($parser_string eq "" ? "":", ") . $parser_tmp;
    }

    return $parser_string;
}

sub create_file_header
{
    my $filehead;
    $filehead  = "/*---------------------------------------------------------------------------\n";
    $filehead .= " * This file is autogenerated file using huawei LCD parser. Please do not edit it.\n";
    $filehead .= " * Update input XML file to add a new entry or update variable in this file\n";
    $filehead .= " * Parser location: vendor/huawei/chipset_common/devkit/lcdkit/tools \n";
    $filehead .= " *---------------------------------------------------------------------------*/\n\n";
    return $filehead;
}


sub create_lcd_enum
{
    my $lcd_enum;
    my $list = shift;
    $lcd_enum  = "\n/*---------------------------------------------------------------------------*/\n";
    $lcd_enum .= "/* static panel selection variable                                           */\n";
    $lcd_enum .= "/*---------------------------------------------------------------------------*/\n";
    $lcd_enum .= "enum {\n";

    foreach my $file_name (@$list) {
        my $temp = $file_name;
        $temp =~ s/-/_/g;  
        $lcd_enum .= uc($temp)."_PANEL,\n";
    }
    #$lcd_enum .= $default_panel . ",\n";
    $lcd_enum .= $invalid_panel . ",\n";
    
    $lcd_enum .= "};\n\n";

    return $lcd_enum;
}

sub create_lcd_map_struct
{
    my $lcd_struct;
    my $name_list = shift;
    $lcd_struct  = "\n/*---------------------------------------------------------------------------*/\n";
    $lcd_struct .= "/* static panel board mapping variable                                           */\n";
    $lcd_struct .= "/*---------------------------------------------------------------------------*/\n";
    $lcd_struct .= "struct lcdkit_board_map {\n";
    $lcd_struct .= "    uint16_t lcd_id;\n";
    $lcd_struct .= "    uint8_t gpio_id;\n";
    $lcd_struct .= "    uint16_t board_id;\n";
    $lcd_struct .= "};\n\n";

    $lcd_struct .= "static struct lcdkit_board_map lcdkit_map[] = {\n";

    foreach my $lcd_name (@$name_list) {
        $lcd_struct .= "    {" . $lcd_name . "},\n";
    }

    $lcd_struct .= "};\n\n";

    return $lcd_struct;
}

sub create_lcd_effect_map_struct
{
    my $lcd_struct;
    my $name_list = shift;
    $lcd_struct  = "\n/*---------------------------------------------------------------------------*/\n";
    $lcd_struct .= "/* static panel board mapping variable                                           */\n";
    $lcd_struct .= "/*---------------------------------------------------------------------------*/\n";
    $lcd_struct .= "struct lcdkit_board_map {\n";
    $lcd_struct .= "    uint16_t lcd_id;\n";
    $lcd_struct .= "    char *panel_compatible;\n";
	$lcd_struct .= "    uint32_t product_id;\n";
    $lcd_struct .= "};\n\n";

    $lcd_struct .= "static struct lcdkit_board_map lcdkit_map[] = {\n";

    foreach my $lcd_name (@$name_list) {
        $lcd_struct .= "    {" . $lcd_name . "},\n";
    }

    $lcd_struct .= "};\n\n";

    return $lcd_struct;
}

#struc on hisi and qcom platform is different, so two function here!

sub create_qcom_lcd_config_data
{
    my $lcd_config;
    my $lcd_name = shift;
    $lcd_config  = "        panelstruct->paneldata    = &".lc($lcd_name)."_panel_data;\n";
    $lcd_config .= "        panelstruct->panelres     = &".lc($lcd_name)."_panel_res;\n";
    $lcd_config .= "        panelstruct->color        = &".lc($lcd_name)."_color;\n";
    $lcd_config .= "        panelstruct->videopanel   = &".lc($lcd_name)."_video_panel;\n";
    $lcd_config .= "        panelstruct->commandpanel = &".lc($lcd_name)."_command_panel;\n";
    $lcd_config .= "        panelstruct->state        = &".lc($lcd_name)."_state;\n";
    $lcd_config .= "        panelstruct->laneconfig   = &".lc($lcd_name)."_lane_config;\n";
    $lcd_config .= "        panelstruct->paneltiminginfo    = &".lc($lcd_name)."_timing_info;\n";
    $lcd_config .= "        panelstruct->panelresetseq = &".lc($lcd_name)."_reset_seq;\n";
    $lcd_config .= "        panelstruct->backlightinfo = &".lc($lcd_name)."_backlight;\n";
    $lcd_config .= "        if(".uc($lcd_name).uc("_bias_ic_array")." == 0)\n";
	$lcd_config .= "        {\n";
	$lcd_config .= "            panelstruct->lcd_bias_ic_info.lcd_bias_ic_list = "."NULL;\n";
	$lcd_config .= "        }\n";
	$lcd_config .= "        else\n";
	$lcd_config .= "        {\n";
	$lcd_config .= "            panelstruct->lcd_bias_ic_info.lcd_bias_ic_list = ".lc($lcd_name)."_bias_ic_array;\n";
	$lcd_config .= "        }\n";
	$lcd_config .= "        panelstruct->lcd_bias_ic_info.num_of_lcd_bias_ic_list = ".uc($lcd_name).uc("_bias_ic_array").";\n";
	$lcd_config .= "        if(".uc($lcd_name).uc("_backlight_ic_array")." == 0)\n";
	$lcd_config .= "        {\n";
	$lcd_config .= "            panelstruct->lcd_backlight_ic_info.lcd_backlight_ic_list = "."NULL;\n";
	$lcd_config .= "        }\n";
	$lcd_config .= "        else\n";
	$lcd_config .= "        {\n";
	$lcd_config .= "            panelstruct->lcd_backlight_ic_info.lcd_backlight_ic_list = ".lc($lcd_name)."_backlight_ic_array;\n";
	$lcd_config .= "        }\n";
	$lcd_config .= "        panelstruct->lcd_backlight_ic_info.num_of_lcd_backlight_ic_list = ".uc($lcd_name).uc("_backlight_ic_array").";\n";
    $lcd_config .= "        pinfo->labibb = &".lc($lcd_name)."_labibb;\n";
    $lcd_config .= "        pinfo->mipi.panel_on_cmds  = ".lc($lcd_name)."_on_command;\n";
    $lcd_config .= "        pinfo->mipi.num_of_panel_on_cmds = ARRAY_SIZE(".lc($lcd_name)."_on_command);\n";
    $lcd_config .= "        pinfo->mipi.panel_off_cmds  = ".lc($lcd_name)."_off_command;\n";
    $lcd_config .= "        pinfo->mipi.num_of_panel_off_cmds = ARRAY_SIZE(".lc($lcd_name)."_off_command);\n";
    $lcd_config .= "        memcpy(phy_db->timing, ".lc($lcd_name)."_timings, sizeof(".lc($lcd_name)."_timings));\n";
    $lcd_config .= "        memcpy(panel_regulator_settings, ".lc($lcd_name)."_regulator_setting, sizeof(".lc($lcd_name)."_regulator_setting));\n";
    $lcd_config .= "        pinfo->pipe_type = ".lc($lcd_name)."_mdp_pipe_type;\n";
    $lcd_config .= "        phy_db->pll_type = ".lc($lcd_name)."_dsi_pll_type;\n";
    $lcd_config .= "        pinfo->mipi.signature = ".lc($lcd_name)."_mipi_signature;\n";
    $lcd_config .= "        lcdkit_config.lcd_platform   = &".lc($lcd_name)."_panel_platform_config;\n";
    $lcd_config .= "        lcdkit_config.lcd_misc   = &".lc($lcd_name)."_panel_misc_info;\n";
    $lcd_config .= "        lcdkit_config.lcd_delay     = &".lc($lcd_name)."_panel_delay_ctrl;\n";
    $lcd_config .= "        pinfo->mipi.tx_eot_append = lcdkit_config.lcd_misc->tx_eot_append;\n";
    $lcd_config .= "        pinfo->mipi.rx_eot_ignore = lcdkit_config.lcd_misc->rx_eot_ignore;\n";
    $lcd_config .= "        phy_db->regulator_mode = lcdkit_config.lcd_misc->mipi_regulator_mode;\n";
    $lcd_config .= "        lcdkit_config.backlight_cmds   = ".lc($lcd_name)."_backlight_command;\n";
    $lcd_config .= "        lcdkit_config.num_of_backlight_cmds   = ARRAY_SIZE(".lc($lcd_name)."_backlight_command);\n";
    $lcd_config .= "        break;\n\n";

    return $lcd_config;
}

sub create_hisi_lcd_config_data
{
    my $lcd_config;
    my $lcd_name = shift;
    $lcd_config  = "        panelstruct->panel   = &".lc($lcd_name)."_panel_info;\n";
    $lcd_config .= "        panelstruct->ldi     = &".lc($lcd_name)."_panel_ldi;\n";
    $lcd_config .= "        panelstruct->mipi    = &".lc($lcd_name)."_panel_mipi;\n";
    $lcd_config .= "        pinfo->display_on_cmds.cmds_set = ".lc($lcd_name)."_on_cmds;\n";
    $lcd_config .= "        pinfo->display_on_cmds.cmd_cnt  = ARRAY_SIZE(".lc($lcd_name)."_on_cmds);\n";
    $lcd_config .= "        pinfo->display_on_second_cmds.cmds_set = ".lc($lcd_name)."_on_second_cmds;\n";
    $lcd_config .= "        pinfo->display_on_second_cmds.cmd_cnt  = ARRAY_SIZE(".lc($lcd_name)."_on_second_cmds);\n";
    $lcd_config .= "        pinfo->display_off_cmds.cmds_set = ".lc($lcd_name)."_off_cmds;\n";
    $lcd_config .= "        pinfo->display_off_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_off_cmds);\n";
    $lcd_config .= "        pinfo->display_off_second_cmds.cmds_set = ".lc($lcd_name)."_off_second_cmds;\n";
    $lcd_config .= "        pinfo->display_off_second_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_off_second_cmds);\n";
    $lcd_config .= "        pinfo->tp_color_cmds.cmds_set = ".lc($lcd_name)."_tp_color_cmds;\n";
    $lcd_config .= "        pinfo->tp_color_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_tp_color_cmds);\n";
    $lcd_config .= "        pinfo->lcd_oemprotectoffpagea.cmds_set = ".lc($lcd_name)."_lcd_oemprotectoffpagea_cmds;\n";
    $lcd_config .= "        pinfo->lcd_oemprotectoffpagea.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_lcd_oemprotectoffpagea_cmds);\n";
    $lcd_config .= "        pinfo->lcd_oemreadfirstpart.cmds_set = ".lc($lcd_name)."_lcd_oemreadfirstpart_cmds;\n";
    $lcd_config .= "        pinfo->lcd_oemreadfirstpart.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_lcd_oemreadfirstpart_cmds);\n";
    $lcd_config .= "        pinfo->lcd_oemprotectoffpageb.cmds_set = ".lc($lcd_name)."_lcd_oemprotectoffpageb_cmds;\n";
    $lcd_config .= "        pinfo->lcd_oemprotectoffpageb.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_lcd_oemprotectoffpageb_cmds);\n";
    $lcd_config .= "        pinfo->lcd_oemreadsecondpart.cmds_set = ".lc($lcd_name)."_lcd_oemreadsecondpart_cmds;\n";
    $lcd_config .= "        pinfo->lcd_oemreadsecondpart.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_lcd_oemreadsecondpart_cmds);\n";
    $lcd_config .= "        pinfo->lcd_oembacktouser.cmds_set = ".lc($lcd_name)."_lcd_oembacktouser_cmds;\n";
    $lcd_config .= "        pinfo->lcd_oembacktouser.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_lcd_oembacktouser_cmds);\n";
    $lcd_config .= "        pinfo->id_pin_read_cmds.cmds_set = ".lc($lcd_name)."_id_pin_check_cmds;\n";
    $lcd_config .= "        pinfo->id_pin_read_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_id_pin_check_cmds);\n";
    $lcd_config .= "        pinfo->color_coordinate_enter_cmds.cmds_set = ".lc($lcd_name)."_color_coordinate_enter_cmds;\n";
    $lcd_config .= "        pinfo->color_coordinate_enter_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_color_coordinate_enter_cmds);\n";
    $lcd_config .= "        pinfo->color_coordinate_cmds.cmds_set = ".lc($lcd_name)."_color_coordinate_cmds;\n";
    $lcd_config .= "        pinfo->color_coordinate_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_color_coordinate_cmds);\n";
    $lcd_config .= "        pinfo->color_coordinate_exit_cmds.cmds_set = ".lc($lcd_name)."_color_coordinate_exit_cmds;\n";
    $lcd_config .= "        pinfo->color_coordinate_exit_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_color_coordinate_exit_cmds);\n";
    $lcd_config .= "        pinfo->panel_info_consistency_enter_cmds.cmds_set = ".lc($lcd_name)."_panel_info_consistency_enter_cmds;\n";
    $lcd_config .= "        pinfo->panel_info_consistency_enter_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_panel_info_consistency_enter_cmds);\n";
    $lcd_config .= "        pinfo->panel_info_consistency_cmds.cmds_set = ".lc($lcd_name)."_panel_info_consistency_cmds;\n";
    $lcd_config .= "        pinfo->panel_info_consistency_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_panel_info_consistency_cmds);\n";
    $lcd_config .= "        pinfo->panel_info_consistency_exit_cmds.cmds_set = ".lc($lcd_name)."_panel_info_consistency_exit_cmds;\n";
    $lcd_config .= "        pinfo->panel_info_consistency_exit_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_panel_info_consistency_exit_cmds);\n";
    $lcd_config .= "        pinfo->display_on_in_backlight_cmds.cmds_set = ".lc($lcd_name)."_display_on_in_backlight_cmds;\n";
    $lcd_config .= "        pinfo->display_on_in_backlight_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_display_on_in_backlight_cmds);\n";
    $lcd_config .= "        pinfo->display_on_effect_cmds.cmds_set = ".lc($lcd_name)."_display_on_effect_cmds;\n";
    $lcd_config .= "        pinfo->display_on_effect_cmds.cmd_cnt = ARRAY_SIZE(".lc($lcd_name)."_display_on_effect_cmds);\n";
    $lcd_config .= "        pinfo->backlight_cmds.cmds_set = ".lc($lcd_name)."_backlight_cmds;\n";
    $lcd_config .= "        pinfo->backlight_cmds.cmd_cnt  = ARRAY_SIZE(".lc($lcd_name)."_backlight_cmds);\n"; 
    $lcd_config .= "        pinfo->lcd_platform   = &".lc($lcd_name)."_panel_platform_config;\n";
    $lcd_config .= "        pinfo->lcd_misc   = &".lc($lcd_name)."_panel_misc_info;\n";
	$lcd_config .= "        pinfo->lcd_delay     = &".lc($lcd_name)."_panel_delay_ctrl;\n";
    $lcd_config .= "        if(".uc($lcd_name).uc("_bias_ic_array")." == 0)\n";
	$lcd_config .= "        {\n";
	$lcd_config .= "            pinfo->lcd_bias_ic_info.lcd_bias_ic_list = "."NULL;\n";
	$lcd_config .= "        }\n";
	$lcd_config .= "        else\n";
	$lcd_config .= "        {\n";
	$lcd_config .= "            pinfo->lcd_bias_ic_info.lcd_bias_ic_list = ".lc($lcd_name)."_bias_ic_array;\n";
	$lcd_config .= "        }\n";
	$lcd_config .= "        pinfo->lcd_bias_ic_info.num_of_lcd_bias_ic_list = ".uc($lcd_name).uc("_bias_ic_array").";\n";
	$lcd_config .= "        if(".uc($lcd_name).uc("_backlight_ic_array")." == 0)\n";
	$lcd_config .= "        {\n";
	$lcd_config .= "            pinfo->lcd_backlight_ic_info.lcd_backlight_ic_list = "."NULL;\n";
	$lcd_config .= "        }\n";
	$lcd_config .= "        else\n";
	$lcd_config .= "        {\n";
	$lcd_config .= "            pinfo->lcd_backlight_ic_info.lcd_backlight_ic_list = ".lc($lcd_name)."_backlight_ic_array;\n";
	$lcd_config .= "        }\n";
	$lcd_config .= "        pinfo->lcd_backlight_ic_info.num_of_lcd_backlight_ic_list = ".uc($lcd_name).uc("_backlight_ic_array").";\n";
    $lcd_config .= "        break;\n\n";
    return $lcd_config;
}

sub create_data_init_func
{
    my $count;
    my $lcd_init;
    my $list = shift;
    my $chip_plat = shift;

    if ($chip_plat eq "qcom"){
        $lcd_init  = "static bool hw_init_panel_data(struct panel_struct *panelstruct,\n";
        $lcd_init .= "            struct msm_panel_info *pinfo,\n";
        $lcd_init .= "            struct mdss_dsi_phy_ctrl *phy_db,\n";
    }
    else
    {
        $lcd_init  = "static bool hw_init_panel_data(struct lcdkit_panel_data *panelstruct,\n";
        $lcd_init .= "            struct lcdkit_disp_info *pinfo,\n";
    }    
    $lcd_init .= "            uint16_t panel_id)\n";
    
    $lcd_init .= "{\n";
    $lcd_init .= "    switch (panel_id) {\n";

    for $count ( 0 ..  scalar(@$list)-1 ) {
        my $temp = @$list[$count];
        $temp =~ s/-/_/g;
        $lcd_init .= "    case " . uc($temp) . "_PANEL:\n";
        if ($chip_plat eq "qcom"){
          $lcd_init .= create_qcom_lcd_config_data($temp);
        }
        else
        {
          $lcd_init .= create_hisi_lcd_config_data($temp);
        }
    }

    #$lcd_init .= "    case " . $default_panel . "_PANEL:\n";
    #if ($chip_plat eq "qcom"){
    #    $lcd_init .= create_qcom_lcd_config_data($default_panel);
    #}
    #else
    #{
    #    $lcd_init .= create_hisi_lcd_config_data($default_panel);
    #}

   # if ($chip_plat eq "qcom"){
        $lcd_init .= "    default:\n";
    #    $lcd_init .= "        ".'dprintf(CRITICAL, "Panel ID not detected %d\n", panel_id);'."\n";
        $lcd_init .= "        return false;\n";
   # }
  #  else
   # {
    #    $lcd_init .= "    default:\n";
     #   $lcd_init .= "        PRINT_INFO(\"Panel ID not detected %d\\n\", panel_id);\n";
      #  $lcd_init .= "        return FALSE;\n";
    #}
    
    $lcd_init .= "    }\n\n";
 #   if ($chip_plat eq "qcom"){
        $lcd_init .= "    return true;\n";
 #   }
  #  else
  #  {
   #     $lcd_init .= "    return TRUE;\n";
  #  }
  
    $lcd_init .= "}\n\n";

    return $lcd_init;
}


sub create_lcd_effect_config
{
    my $lcd_effect_cfg;
    my $lcd_name = shift;
    $lcd_effect_cfg  = "        if (pinfo->acm_support == 1)\n";
    $lcd_effect_cfg .= "        {\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_hue_table = " . lc($lcd_name) . "_acm_lut_hue_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_hue_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_hue_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_sata_table = " . lc($lcd_name) . "_acm_lut_sata_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_sata_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_sata_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr_table = " . lc($lcd_name) . "_acm_lut_satr_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr0_table = " . lc($lcd_name) . "_acm_lut_satr0_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr0_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr0_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr1_table = " . lc($lcd_name) . "_acm_lut_satr1_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr1_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr1_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr2_table = " . lc($lcd_name) . "_acm_lut_satr2_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr2_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr2_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr3_table = " . lc($lcd_name) . "_acm_lut_satr3_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr3_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr3_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr4_table = " . lc($lcd_name) . "_acm_lut_satr4_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr4_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr4_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr5_table = " . lc($lcd_name) . "_acm_lut_satr5_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr5_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr5_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr6_table = " . lc($lcd_name) . "_acm_lut_satr6_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr6_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr6_table);\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr7_table = " . lc($lcd_name) . "_acm_lut_satr7_table;\n";
    $lcd_effect_cfg .= "            pinfo->acm_lut_satr7_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_acm_lut_satr7_table);\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_hue_table = " . lc($lcd_name) . "_video_acm_lut_hue_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_sata_table = " . lc($lcd_name) . "_video_acm_lut_sata_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_satr0_table = " . lc($lcd_name) . "_video_acm_lut_satr0_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_satr1_table = " . lc($lcd_name) . "_video_acm_lut_satr1_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_satr2_table = " . lc($lcd_name) . "_video_acm_lut_satr2_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_satr3_table = " . lc($lcd_name) . "_video_acm_lut_satr3_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_satr4_table = " . lc($lcd_name) . "_video_acm_lut_satr4_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_satr5_table = " . lc($lcd_name) . "_video_acm_lut_satr5_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_satr6_table = " . lc($lcd_name) . "_video_acm_lut_satr6_table;\n";
    $lcd_effect_cfg .= "            pinfo->video_acm_lut_satr7_table = " . lc($lcd_name) . "_video_acm_lut_satr7_table;\n";
    $lcd_effect_cfg .= "        }\n";
  
    $lcd_effect_cfg .= "        if (pinfo->gamma_support == 1)\n";
    $lcd_effect_cfg .= "        {\n";
    $lcd_effect_cfg .= "            pinfo->gamma_lut_table_R = " . lc($lcd_name) . "_gamma_lut_table_R;\n";
    $lcd_effect_cfg .= "            pinfo->gamma_lut_table_G = " . lc($lcd_name) . "_gamma_lut_table_G;\n";
    $lcd_effect_cfg .= "            pinfo->gamma_lut_table_B = " . lc($lcd_name) . "_gamma_lut_table_B;\n";
    $lcd_effect_cfg .= "            pinfo->gamma_lut_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_gamma_lut_table_R);\n";
    $lcd_effect_cfg .= "            pinfo->igm_lut_table_R = " . lc($lcd_name) . "_igm_lut_table_R;\n";
    $lcd_effect_cfg .= "            pinfo->igm_lut_table_G = " . lc($lcd_name) . "_igm_lut_table_G;\n";
    $lcd_effect_cfg .= "            pinfo->igm_lut_table_B = " . lc($lcd_name) . "_igm_lut_table_B;\n";
    $lcd_effect_cfg .= "            pinfo->igm_lut_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_igm_lut_table_R);\n";
    #$lcd_effect_cfg .= "            pinfo->gmp_support = 1;\n";
    $lcd_effect_cfg .= "            pinfo->gmp_lut_table_low32bit = &" . lc($lcd_name) . "_gmp_lut_table_low32bit[0][0][0];\n";
    $lcd_effect_cfg .= "            pinfo->gmp_lut_table_high4bit = &" . lc($lcd_name) . "_gmp_lut_table_high4bit[0][0][0];\n";
    $lcd_effect_cfg .= "            pinfo->gmp_lut_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_gmp_lut_table_low32bit);\n";
    #$lcd_effect_cfg .= "            pinfo->xcc_support = 1;\n";
    $lcd_effect_cfg .= "            pinfo->xcc_table = " . lc($lcd_name) . "_xcc_table;\n";
    $lcd_effect_cfg .= "            pinfo->xcc_table_len = ARRAY_SIZE(" . lc($lcd_name) . "_xcc_table);\n";
    $lcd_effect_cfg .= "        }\n";
    $lcd_effect_cfg .= "        break;\n";
    
    return $lcd_effect_cfg;
}

sub create_lcd_effect_data_func
{
    my $lcd_effect_func;
    my $list = shift;
    my $i;

    $lcd_effect_func  = "static void lcdkit_effect_get_data(uint8_t panel_id, struct hisi_panel_info* pinfo)\n";
    $lcd_effect_func .= "{\n";
    $lcd_effect_func .= "    switch (panel_id) {\n";

    for $i ( 0 ..  scalar(@$list)-1 ) {
        my $temp = @$list[$i];
        $temp =~ s/-/_/g;
        $lcd_effect_func .= "    case " . uc($temp) . "_PANEL:\n";
        $lcd_effect_func .= create_lcd_effect_config($temp);
    }

    # $lcd_effect_func .= "    case ".$default_panel."_PANEL:\n";
    # $lcd_effect_func .= create_lcd_effect_config($default_panel);

    $lcd_effect_func .= "    default:\n";
    $lcd_effect_func .= "        ".'HISI_FB_INFO("Panel ID not detected %d\n", panel_id);'."\n";
    $lcd_effect_func .= "        break;\n";
    $lcd_effect_func .= "    }\n";
    $lcd_effect_func .= "}\n\n";

    return $lcd_effect_func;
}

sub create_hisi_panel_init
{
    my $panel_init;
    $panel_init  = "static uint8_t lcdkit_panel_init(uint16_t product_id)\n";
    $panel_init .= "{\n";
    $panel_init .= "    uint8_t lcd_panel_id = " . $default_panel . ";\n";
    #$panel_init .= "    uint8_t lcd_panel_id = " . $invalid_panel . ";\n";
	$panel_init .= "    int lcdkit_id;\n";
    $panel_init .= "    uint32_t i = 0;\n";
	$panel_init .= "    lcdkit_id = get_lcdkit_id();\n\n";
 #   $panel_init .= "    PRINT_INFO(\"lcd_panel_init lcd id = %d\\n\", lcd_id);\n";

    $panel_init .= "    for (i = 0; i < ARRAY_SIZE(lcdkit_map); ++i) {\n";
    $panel_init .= "        if ((lcdkit_map[i].board_id == product_id) && (lcdkit_map[i].gpio_id == lcdkit_id)) {\n";
    $panel_init .= "            lcd_panel_id = lcdkit_map[i].lcd_id;\n";
    $panel_init .= "            break;\n";
    $panel_init .= "        }\n";
    $panel_init .= "    }\n\n";

    $panel_init .= "    return lcd_panel_id;\n";
    $panel_init .= "}\n\n";

    return $panel_init;
}

sub create_hisi_panel_effect_init
{
    my $panel_init;
    $panel_init  = "static uint8_t lcdkit_panel_init(uint32_t product_id, char* compatible)\n";
    $panel_init .= "{\n";
    #$panel_init .= "    uint8_t lcd_panel_id = " . $invalid_panel . ";\n";
	$panel_init .= "    uint8_t lcd_panel_id = " . $default_panel . ";\n";
    $panel_init .= "    int i = 0;\n";
	$panel_init .= "    int len = strlen(compatible);\n";
	$panel_init .= "    int length = 0;\n";

    $panel_init .= "    for (i = 0; i < ARRAY_SIZE(lcdkit_map); ++i) {\n";
	$panel_init .= "        length = strlen(lcdkit_map[i].panel_compatible);\n";
    $panel_init .= "        if ((lcdkit_map[i].product_id == product_id) && !strncmp(lcdkit_map[i].panel_compatible, compatible, (length > len?length:len))){\n";
    $panel_init .= "            lcd_panel_id = lcdkit_map[i].lcd_id;\n";
    $panel_init .= "            break;\n";
    $panel_init .= "        }\n";
    $panel_init .= "    }\n\n";
	$panel_init .= "    " . 'HISI_FB_INFO("lcd_panel_id = %d\n", lcd_panel_id);' . "\n";
    $panel_init .= "    return lcd_panel_id;\n";
    $panel_init .= "}\n\n";

    return $panel_init;
}

sub create_qcom_panel_init
{
    my $panel_init;
    $panel_init  = "static uint16_t lcdkit_panel_init(uint16_t hw_id)\n";
    $panel_init .= "{\n";
    $panel_init .= "    uint16_t lcd_panel_id = " . $default_panel . ";\n";
    #$panel_init .= "    uint8_t lcd_panel_id = " . $invalid_panel . ";\n";
    $panel_init .= "    hw_lcd_id_index lcd_hw_id = LCD_HW_ID_MAX;\n";
    $panel_init .= "    uint32_t i = 0;\n";
    $panel_init .= "    lcd_hw_id = (hw_lcd_id_index)hw_get_lcd_id(hw_id);\n\n";
    $panel_init .= "    ".'dprintf(INFO,"lcd_panel_init lcd id = %d\n", lcd_hw_id);'."\n";
    $panel_init .= "    ".'snprintf(lcd_id,sizeof(lcd_id), " huawei,lcd_panel_id = %X", lcd_hw_id);'."\n";

    $panel_init .= "    for (i = 0; i < ARRAY_SIZE(lcdkit_map); ++i) {\n";
    $panel_init .= "        if ((lcdkit_map[i].board_id == hw_id) && (lcdkit_map[i].gpio_id == lcd_hw_id)) {\n";
    $panel_init .= "            lcd_panel_id = lcdkit_map[i].lcd_id;\n";
    $panel_init .= "            break;\n";
    $panel_init .= "        }\n";
    $panel_init .= "    }\n\n";

    $panel_init .= "    return lcd_panel_id;\n";
    $panel_init .= "}\n\n";

    return $panel_init;
}


sub create_qcom_gpio_init
{
    my $gpio_init;
	my $def_idgpio = shift;
	my $spec_idgpio = shift;
	
	$def_idgpio =~ s/ //g;
	$spec_idgpio =~ s/ //g;
	$spec_idgpio = apply_patten($spec_idgpio);
	if ($parse_error_string eq $spec_idgpio)
	{
        $spec_idgpio = "0,0,0";
	}
	
	my @element = split /\;/, $spec_idgpio;
	
	my $temp = 0;
	$spec_idgpio = "";
	for(my $count = 0; $count < @element; $count++)
	{
		if (length($spec_idgpio) >= 27 + 68 * $temp)
		{
            $temp++;
            $spec_idgpio .= "\n            ";
		}
		
        $spec_idgpio .= "{" . $element[$count] . "}, ";		
	}
	chop($spec_idgpio);
	chop($spec_idgpio);
    
    $gpio_init  = "static void lcdkit_get_id_gpio(uint16_t hw_id, int* id0, int* id1)\n";
    $gpio_init .= "{\n";
	$gpio_init .= "    uint16_t i = 0;\n";
    $gpio_init .= "    uint16_t def_id[] = {" . $def_idgpio . "};\n";
    $gpio_init .= "    uint16_t id_gpio_grp[" . @element . "][3] = {" . $spec_idgpio . "};\n\n";
    $gpio_init .= "    for (i = 0; i < " . @element . "; i++)\n";
	$gpio_init .= "    {\n";
    $gpio_init .= "        if (hw_id == id_gpio_grp[i][2])\n";
	$gpio_init .= "        {\n";
    $gpio_init .= "            *id0 = id_gpio_grp[i][0];\n";
	$gpio_init .= "            *id1 = id_gpio_grp[i][1];\n";
	$gpio_init .= "            return;\n";
	$gpio_init .= "        }\n\n";
	$gpio_init .= "    }\n\n";
    $gpio_init .= "    if (i >= " . @element . ")\n";
	$gpio_init .= "    {\n";
    $gpio_init .= "        *id0 = def_id[0];\n";
	$gpio_init .= "        *id1 = def_id[1];\n";
	$gpio_init .= "    }\n\n";
	$gpio_init .= "    return;\n";
    $gpio_init .= "}\n\n";

    return $gpio_init;
}



1;