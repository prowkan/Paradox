package ui.controls
{
	import flash.display.MovieClip;
	import flash.events.MouseEvent;

	public class Button extends MovieClip
	{
		public function Button(): void
		{
			this.ButtonTextValue = "TextField";
			
			this.addEventListener(MouseEvent.MOUSE_OVER, function() {
				gotoAndStop("hovered");
			});
			
			this.addEventListener(MouseEvent.MOUSE_OUT, function() {
				gotoAndStop("default");
			});			
		}
		
		public var ButtonTextValue: String;
	};
}
