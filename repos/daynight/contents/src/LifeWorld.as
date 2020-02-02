package
{
	import net.flashpunk.World;
	
	public class LifeWorld extends World
	{
		public function LifeWorld() 
		{
			add(new Swimmer);
		}
		
	}
}