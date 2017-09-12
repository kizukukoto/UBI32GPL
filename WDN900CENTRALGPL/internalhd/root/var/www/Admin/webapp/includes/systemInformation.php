<?php

class SystemInformation{

    function SystemInformation() {
    }
    
    function getInfo(){
        $nasConfig = @parse_ini_file('/etc/nasglobalconfig.ini', true);
        $useShellScriptForTotalSpace = false;
        $useShellScriptForNasInfo = false;
        if ($nasConfig === false)
        {
            $useShellScriptForTotalSpace = true;
            $useShellScriptForNasInfo = true;
        }
        else 
        {
           if (!isset($nasConfig['admin']['DATA_VOLUME']))
           {
               $useShellScriptForTotalSpace = true;
           } 
           if (!isset($nasConfig['admin']['NAS_DEVICE_INFO_FILE']))
           {
               $useShellScriptForNasInfo = true;
           }
        }
        if (SKIP_SHELL_SCRIPT && !$useShellScriptForTotalSpace)
        {
            $diskTotalSpace = disk_total_space($nasConfig['admin']['DATA_VOLUME']);
            $size = ceil((float)$diskTotalSpace/1024/1024/1024);
            $size += 500;
            $remainder = $size%1000;
            $whole = (int)($size/1000);
            if ($remainder > 500)
            {
            	$capacity = $whole.'.5TB';
            }
            else
            {
            	$capacity = $whole.'TB';
            }
        }
        else
        {
            unset($capacity);
            exec("sudo /usr/local/sbin/getSystemCapacity.sh", $capacity, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            if((!isset($capacity[0])) || ($capacity[0] == "")){
                $capacity[0] = "0";
            }
            $capacity = $capacity[0];
        }
        if (SKIP_SHELL_SCRIPT && !$useShellScriptForNasInfo)
        {
            $xml = @simplexml_load_file($nasConfig['admin']['NAS_DEVICE_INFO_FILE']);
            if($xml === false)
            {
                $useShellScriptForNasInfo = true;
            }
            else
            {
            	$manufacturer = (string)$xml->device->manufacturer;
            	$manufacturer_url = (string)$xml->device->manufacturerURL;
            	$model_description = (string)$xml->device->modelDescription;
            	$model_name = (string)$xml->device->modelName;
            	$model_url = (string)$xml->device->modelURL;
            	$model_number = (string)$xml->device->modelNumber;
            	$serial_number = (string)$xml->device->serialNumber;
            }
        }
        if (!SKIP_SHELL_SCRIPT || $useShellScriptForNasInfo)
        {
            unset($manufacturer);
            exec('sudo sed -n -e \'/<manufacturer>/s/\(.*\)\(<manufacturer>\)\(.*\)\(<\/manufacturer>\)/\3/p\' /etc/nas/nasdevice.xml', $manufacturer, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $manufacturer = $manufacturer[0];
    
            unset($manufacturer_url);
            exec('sudo sed -n -e \'/<manufacturerURL>/s/\(.*\)\(<manufacturerURL>\)\(.*\)\(<\/manufacturerURL>\)/\3/p\' /etc/nas/nasdevice.xml', $manufacturer_url, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $manufacturer_url = $manufacturer_url[0];
    
            unset($model_description);
            exec('sudo sed -n -e \'/<modelDescription>/s/\(.*\)\(<modelDescription>\)\(.*\)\(<\/modelDescription>\)/\3/p\' /etc/nas/nasdevice.xml', $model_description, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $model_description = $model_description[0];
    
            unset($model_name);
            exec('sudo sed -n -e \'/<modelName>/s/\(.*\)\(<modelName>\)\(.*\)\(<\/modelName>\)/\3/p\' /etc/nas/nasdevice.xml', $model_name, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $model_name = $model_name[0];
    
            unset($model_url);
            exec('sudo sed -n -e \'/<modelURL>/s/\(.*\)\(<modelURL>\)\(.*\)\(<\/modelURL>\)/\3/p\' /etc/nas/nasdevice.xml', $model_url, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $model_url = $model_url[0];
    
            unset($model_number);
            exec('sudo sed -n -e \'/<modelNumber>/s/\(.*\)\(<modelNumber>\)\(.*\)\(<\/modelNumber>\)/\3/p\' /etc/nas/nasdevice.xml', $model_number, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $model_number = $model_number[0];
            
            unset($serial_number);
            exec('sudo sed -n -e \'/<serialNumber>/s/\(.*\)\(<serialNumber>\)\(.*\)\(<\/serialNumber>\)/\3/p\' /etc/nas/nasdevice.xml', $serial_number, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $serial_number = $serial_number[0];
        }
        
        unset($mac_address);
        if (SKIP_SHELL_SCRIPT)
        {
            $text = `ifconfig`;
            preg_match('/([0-9a-f]{2}:){5}\w\w/i', $text, $mac);
            $mac_address = $mac[0];    
        }
        else 
        {
            unset($mac_address);
            exec("sudo /usr/local/sbin/getMacAddress.sh", $mac_address, $retVal);
            if($retVal !== 0) {
                return NULL;
            }
            $mac_address = $mac_address[0];
        }
        

/*
        return( array(
                    'manufacturer' => "$manufacturer[0]",
                    'manufacturer_url' => "$manufacturer_url[0]", 
                    'model_description' => "$model_description[0]",
                    'model_name' => "$model_name[0]",
                    'model_url' => "$model_url[0]",
                    'model_number' => "$model_number[0]",
                    'capacity' => "$capacity[0]",
                    'serial_number' => "$serial_number[0]",
                    'mac_address' => "$mac_address[0]",
                    )
            );
*/
        return( array(
                    'manufacturer' => $manufacturer,
                    'manufacturer_url' => $manufacturer_url,
                    'model_description' => $model_description,
                    'model_name' => $model_name,
                    'model_url' => $model_url,
                    'model_number' => $model_number,
                    'capacity' => $capacity,
                    'serial_number' => $serial_number,
                    'mac_address' => $mac_address,
                   )
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
