rd  /S /Q Project\MDKARM(uV5)\Listings 

del *.crf /Q /S
del *.o   /Q /S
del *.d   /Q /S
::del *.axf /Q /S
del *.htm /Q /S
del *.sct /Q /S
del *.dep /Q /S

del *.lst /Q /S
del *.map /Q /S

del *.uvguix.*   /Q /S
del JLinkLog.txt /Q /S

del *.iex /s
del *.tra /s
del *.bak /s
del *.ddk /s
del *.edk /s
del *.lnp /s
del *.mpf /s
del *.mpj /s
del *.obj /s
del *.omf /s
::del *.opt /s  ::������ɾ��JLINK������
del *.plg /s
del *.rpt /s
del *.tmp /s
del *.__i /s
rd /q /s MDK-ARM\DebugConfig
rd /q /s MDK-ARM\RTE

del YS-F1Pro.dep /s
rd /q /s EWARM\YS-F1Pro\List
rd /q /s EWARM\YS-F1Pro\Obj
::rd /q /s EWARM\settings

exit