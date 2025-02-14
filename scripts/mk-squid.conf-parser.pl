#!/usr/bin/perl

use strict;
use warnings;
use Getopt::Long qw(:config auto_version auto_help);
use Pod::Usage;

=pod

=head1 NAME

 mk-squid.conf-parser.pl - Generate squid.conf.default and cf_parser.cci

=head1 SYNOPSIS

 mk-squid.conf-parser.pl cf.data cf.data.depend

=head1 DESCRIPTION

This program parses the input file and generates code and
files used to configure the variables in squid.
(i.e. it creates the squid.conf.default file from the cf.data file)

The output files are as follows:

=over 12

=item cf_parser.cci

This file contains,
 - default_all() which initializes variables with the default values,
 - parse_line() that parses line from squid.conf,
 - dump_config() that dumps the current the values of the variables

=item squid.conf.default

Default configuration file given to the server administrator.

=back

=head1 OPTIONS

=over 12

=item B<cf.data>

File containing definitions of all squid.conf directives,
including their documentation, default values, and syntax
to be part of squid.conf.default.

=item B<cf.data.depend>

File containing the type dependency information for
squid.conf directive types used in B<cf.data>.

=back

=head1 AUTHOR

This software is written by Amos Jeffries <amosjeffries@squid-cache.org>

=head1 COPYRIGHT

 * Copyright (C) 1996-2025 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.

=head1 QUESTIONS

Questions on the usage of this program can be sent to the
I<Squid Developers mailing list <squid-dev@lists.squid-cache.org>>

=head1 REPORTING BUGS

Bug reports need to be made in English.
See https://wiki.squid-cache.org/SquidFaq/BugReporting for details of what you need to include with your bug report.

Report bugs or bug fixes using https://bugs.squid-cache.org/

Report serious security bugs to I<Squid Bugs <squid-bugs@lists.squid-cache.org>>

Report ideas for new improvements to the I<Squid Developers mailing list <squid-dev@lists.squid-cache.org>>

=head1 SEE ALSO

squid (8), GPL (7),

The Squid FAQ wiki https://wiki.squid-cache.org/SquidFaq

The Squid Configuration Manual http://www.squid-cache.org/Doc/config/

=cut

use YAML::Tiny;

# Open the config
my $yaml = YAML::Tiny->read( $ARGV[0] );

#print $yaml;
