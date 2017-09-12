<?php

class TimeZones{

    function TimeZones() {
    }
    
    function getTimeZones(){

        return array(
            'Pacific/Midway'                 => '(GMT-11:00) Midway Island, Samoa',
            'US/Hawaii'                      => '(GMT-10:00) Hawaii',
            'US/Alaska'                      => '(GMT-09:00) Alaska',
            'US/Pacific'                     => '(GMT-08:00) Pacific Time (US & Canada)',
            'America/Tijuana'                => '(GMT-08:00) Tijuana',
            'US/Arizona'                     => '(GMT-07:00) Arizona',
            'America/Chihuahua'              => '(GMT-07:00) Chihuahua, La Paz, Mazatlan',
            'US/Mountain'                    => '(GMT-07:00) Mountain Time (US & Canada)',
            'America/Guatemala'              => '(GMT-06:00) Central America (Guatemala)',
            'US/Central'                     => '(GMT-06:00) Central Time (US & Canada)',
            'America/Mexico_City'            => '(GMT-06:00) Guadalajara, Mexico City, Monterrey',
            'Canada/Saskatchewan'            => '(GMT-06:00) Saskatchewan',
            'America/Bogota'                 => '(GMT-05:00) Bogota, Lima, Quito',
            'US/Eastern'                     => '(GMT-05:00) Eastern Time (US & Canada)',
            'US/East-Indiana'                => '(GMT-05:00) Indiana (East)',
            'America/Caracas'                => '(GMT-04:30) Caracas',
            'America/Goose_Bay'              => '(GMT-04:00) Atlantic Time (Canada)',
            'America/La_Paz'                 => '(GMT-04:00) La Paz',
            'America/Santiago'               => '(GMT-04:00) Santiago',
            'America/Manaus'                 => '(GMT-04:00) Manaus',
            'Canada/Newfoundland'            => '(GMT-03:30) Newfoundland',
            'Brazil/East'                    => '(GMT-03:00) Brasilia',
            'America/Argentina/Buenos_Aires' => '(GMT-03:00) Buenos Aires, Georgetown',
            'America/Godthab'                => '(GMT-03:00) Greenland',
            'America/Montevideo'             => '(GMT-03:00) Montevideo',
            'America/Noronha'                => '(GMT-02:00) Mid-Atlantic (Noronha)',
            'Atlantic/Azores'                => '(GMT-01:00) Azores Is.',
            'Atlantic/Cape_Verde'            => '(GMT-01:00) Cape Verde Is.',
            'Africa/Casablanca'              => '(GMT) Casablanca, Monrovia',
            'Europe/London'                  => '(GMT) Greenwich Mean Time: Dublin, Edinburgh, Lisbon, London',
            'Europe/Amsterdam'               => '(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna',
            'Europe/Belgrade'                => '(GMT+01:00) Belgrade, Bratislava, Budapest, Lijublijana, Pargue',
            'Europe/Brussels'                => '(GMT+01:00) Brussels, Copenhagen, Madrid, Paris',
            'Europe/Sarajevo'                => '(GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb',
            'Africa/Niamey'                  => '(GMT+01:00) West Central Africa',
            'Asia/Amman'                     => '(GMT+02:00) Amman',
            'Asia/Beirut'                    => '(GMT+02:00) Beirut',
            'Europe/Minsk'                   => '(GMT+02:00) Minsk',
            'Europe/Athens'                  => '(GMT+02:00) Athens, Bucharest, Istanbul',
            'Africa/Cairo'                   => '(GMT+02:00) Cairo',
            'Africa/Harare'                  => '(GMT+02:00) Harare, Pretoria',
            'Europe/Helsinki'                => '(GMT+02:00) Helsinki, kyiv, Riga, Sofia, Tallinn, Vilnius',
            'Asia/Jerusalem'                 => '(GMT+02:00) Jerusalem',
            'Asia/Baghdad'                   => '(GMT+03:00) Baghdad',
            'Asia/Kuwait'                    => '(GMT+03:00) Kuwait, Riyadh',
            'Europe/Moscow'                  => '(GMT+03:00) Moscow, St. Petersburg, Volgograd',
            'Africa/Nairobi'                 => '(GMT+03:00) Nairobi',
            'Asia/Tehran'                    => '(GMT+03:30) Tehran',
            'Asia/Muscat'                    => '(GMT+04:00) Abu Dhabi, Muscat',
            'Asia/Baku'                      => '(GMT+04:00) Baku, Tbilisi, Yerevan',
            'Asia/Yerevan'                   => '(GMT+04:00) Yerevan',
            'Asia/Kabul'                     => '(GMT+04:30) Kabul',
            'Asia/Yekaterinburg'             => '(GMT+05:00) Ekaterinburg',
            'Asia/Karachi'                   => '(GMT+05:00) Islamabad, Karachi',
            'Asia/Tashkent'                  => '(GMT+05:00) Tashkent',
            'Asia/Kolkata'                   => '(GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi',
            'Asia/Katmandu'                  => '(GMT+05:45) Katmandu',
            'Asia/Almaty'                    => '(GMT+06:00) Almaty, Novosibirsk',
            'Asia/Dhaka'                     => '(GMT+06:00) Astana, Dhaka',
            'Asia/Rangoon'                   => '(GMT+06:30) Rangoon',
            'Asia/Bangkok'                   => '(GMT+07:00) Bangkok, Hanoi, Jakarta',
            'Asia/Krasnoyarsk'               => '(GMT+07:00) Krasnoyarsk',
            'Asia/Hong_Kong'                 => '(GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi',
            'Asia/Irkutsk'                   => '(GMT+08:00) Irkutsk, Ulaan Bataar',
            'Asia/Kuala_Lumpur'              => '(GMT+08:00) Kuala Lumpur, Singapore',
            'Australia/Perth'                => '(GMT+08:00) Perth',
            'Asia/Taipei'                    => '(GMT+08:00) Taipei',
            'Asia/Tokyo'                     => '(GMT+09:00) Osaka, Sapporo, Tokyo',
            'Asia/Seoul'                     => '(GMT+09:00) Seoul',
            'Asia/Yakutsk'                   => '(GMT+09:00) Yakutsk',
            'Australia/Adelaide'             => '(GMT+09:30) Adelaide',
            'Australia/Darwin'               => '(GMT+09:30) Darwin',
            'Australia/Brisbane'             => '(GMT+10:00) Brisbane',
            'Australia/Canberra'             => '(GMT+10:00) Canberra, Melbourne, Sydney',
            'Pacific/Guam'                   => '(GMT+10:00) Guam, Port Moresby',
            'Australia/Hobart'               => '(GMT+10:00) Hobart',
            'Asia/Vladivostok'               => '(GMT+10:00) Vladivostok',
            'Asia/Magadan'                   => '(GMT+11:00) Magadan, Solomon Is., New Caledonia',
            'Pacific/Auckland'               => '(GMT+12:00) Auckland, Wellington',
            'Pacific/Fiji'                   => '(GMT+12:00) Fiji, Kamchatka, Marshall Is.'
            );
    }
  }

/*
 * Local variables:
 *  indent-tabs-mode: nil
 *  c-basic-offset: 4
 *  c-indent-level: 4
 *  tab-width: 4
 * End:
 */
?>