function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();
	  if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/welle-io.exe", "@StartMenuDir@/welle.io.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@//welle-io.exe");
		component.addElevatedOperation("Execute", "@TargetDir@/vcredist_x86_2012.exe", "/quiet", "/norestart");
		component.addElevatedOperation("Delete", "@TargetDir@/vcredist_x86_2012.exe");
		component.addElevatedOperation("Execute", "@TargetDir@/vcredist_x86_2010.exe", "/quiet", "/norestart");
		component.addElevatedOperation("Delete", "@TargetDir@/vcredist_x86_2010.exe");
    }
}
