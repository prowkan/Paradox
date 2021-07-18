package ui.controls
{
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.text.TextField;

	public class Button extends MovieClip
	{
		public function Button(): void
		{
			this.ButtonTextValue = "TextField";
			this.IsButtonPressed = false;
			
			this.addEventListener(MouseEvent.MOUSE_OVER, function() 
			{
				if (!IsButtonPressed)
					gotoAndStop("hovered");
				else
					gotoAndStop("active");
			});
			
			this.addEventListener(MouseEvent.MOUSE_OUT, function() 
			{
				IsButtonPressed = false;
				gotoAndStop("default");
			});
			
			this.addEventListener(MouseEvent.MOUSE_DOWN, function() 
			{
				IsButtonPressed = true;
				gotoAndStop("active");
			});	
			
			this.addEventListener(MouseEvent.MOUSE_UP, function() 
			{
				IsButtonPressed = false;
				gotoAndStop("hovered");
			});	
		}
		
		public var ButtonTextValue: String;		
		public var ButtonTextField: TextField;		
		private var IsButtonPressed: Boolean;
	};
}
