object TPackageNode
  Name = 'Application'
  Description = 'Application Configuration'
  DefineName = 'APPLICATION'
  ConfigType = kAllPublicFiles
  object TYesNoNode
    Name = 'Runtime Debugging'
    Description = 'Enable debug output from the project'#39's main application.'
    DefineName = 'APPLICATION_DEBUG'
    ConfigType = kPrivateHeaderFile
    object TListNode
      Name = 'Compiled Debug Messages'
      Description = 
        'Debug messages at this severity or higher will be compiled in.  ' +
        'This setting affects both static and dynamic modes (regardless o' +
        'f Enable Dynamic DEBUG).'
      DefineName = 'DEBUG_PKG_LVL_MESSAGES'
      ConfigType = kPrivateHeaderFile
      Value = 'Info (3)'
      ListName = 'DEBUG_MACRO_LIST'
    end
    object TYesNoNode
      Name = 'Enable Dynamic DEBUG'
      Description = 
        'Enable runtime debug level control for the application.  If this' +
        ' is not enabled, all debug messages selected with Compiled Debug' +
        ' Messages are always printed.'
      DefineName = 'DEBUG_PKG_DYNAMIC'
      ConfigType = kPrivateHeaderFile
      Value = True
      object TListNode
        Name = 'Initial Dynamic Debug Threshold'
        Description = 
          'This value is used at runtime to determine what debug messages a' +
          're printed. The value assigned here is the initial threshold; th' +
          'e value can be changed at run time by the debugger or possibly b' +
          'y the project'
        DefineName = 'DEBUG_DYN_INIT_THRESHOLD'
        ConfigType = kPrivateHeaderFile
        Value = 'Warn (2)'
        ListName = 'DEBUG_MACRO_LIST'
      end
    end
  end
end
